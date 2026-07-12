// Real SDL3-backed implementation of C_4JInput.
//
// Every other platform (Xbox, PS3, PSVita, Orbis, Windows64/XInput) backs this
// class with a physical gamepad: <Platform>_Minecraft.cpp's main() builds a
// table via SetGameJoypadMaps() mapping abstract MINECRAFT_ACTION_*/
// ACTION_MENU_* actions to _360_JOY_BUTTON_* bitmask tokens (see e.g.
// Windows64_Minecraft.cpp's DefineActions()), and Tick()/GetJoypadStick_*()/
// ButtonDown() etc. read real controller hardware against that table.
//
// There's no physical gamepad on the desktop SDL3 build. This backend
// synthesizes the same virtual-button/virtual-stick model out of the keyboard
// and mouse instead: WASD drives the left (movement) stick, mouse motion
// drives the right (look) stick, and a fixed set of keys/mouse buttons map to
// the button tokens. SDL3_Minecraft.cpp's main() calls SetGameJoypadMaps()
// with the same MAP_STYLE_0 bindings Windows64_Minecraft.cpp uses, so
// everything downstream (Input.cpp's player movement/look, Minecraft::
// run_middle's ullButtonsPressed latch, tooltip button-name lookups, ...)
// works completely unmodified.
#include "../../../stdafx.h"
#include "../../SDL3_Input.h"

namespace
{
constexpr int kMaxMapStyles = 4;
constexpr int kMaxActions = MINECRAFT_ACTION_MAX;

unsigned int s_actionMap[kMaxMapStyles][kMaxActions] = {};
unsigned char s_padMapVal[XUSER_MAX_COUNT] = {};

unsigned int s_buttonsDown = 0;
unsigned int s_buttonsDownPrev = 0;

float s_stickLX = 0.0f;
float s_stickLY = 0.0f;
float s_stickRX = 0.0f;
float s_stickRY = 0.0f;

// Input.cpp applies the user sensitivity and a 50-degree full-stick turn.
// Keep one physical pixel well below full-stick magnitude: the previous
// mapping saturated after only a few pixels and turned modest motion into a snap.
constexpr float kMouseLookScale = 1.0f / 50.0f;

float Clamp(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
} // namespace

C_4JInput InputManager;

void C_4JInput::Initialise(int, unsigned char, unsigned char, unsigned char)
{
    for (auto &map : s_actionMap)
    {
        for (unsigned int &val : map)
        {
            val = 0;
        }
    }
    for (unsigned char &val : s_padMapVal)
    {
        val = 0;
    }
    s_buttonsDown = s_buttonsDownPrev = 0;
    s_stickLX = s_stickLY = s_stickRX = s_stickRY = 0.0f;
}

void C_4JInput::Tick(void)
{
    s_buttonsDownPrev = s_buttonsDown;

    int numKeys = 0;
    const bool *keys = SDL_GetKeyboardState(&numKeys);
    auto down = [&](SDL_Scancode sc) { return keys != nullptr && sc < numKeys && keys[sc]; };

    SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(nullptr, nullptr);
    bool lmb = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
    bool rmb = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

    unsigned int buttons = 0;
    if (down(SDL_SCANCODE_SPACE) || down(SDL_SCANCODE_RETURN))
    {
        buttons |= _360_JOY_BUTTON_A;
    }
    if (down(SDL_SCANCODE_LSHIFT) || down(SDL_SCANCODE_RSHIFT))
    {
        buttons |= _360_JOY_BUTTON_RTHUMB;
    }
    if (down(SDL_SCANCODE_E))
    {
        buttons |= _360_JOY_BUTTON_Y;
    }
    if (down(SDL_SCANCODE_Q))
    {
        buttons |= _360_JOY_BUTTON_B;
    }
    if (down(SDL_SCANCODE_C))
    {
        buttons |= _360_JOY_BUTTON_X;
    }
    if (down(SDL_SCANCODE_F5))
    {
        buttons |= _360_JOY_BUTTON_LTHUMB;
    }
    if (down(SDL_SCANCODE_TAB))
    {
        buttons |= _360_JOY_BUTTON_BACK;
    }
    if (down(SDL_SCANCODE_ESCAPE))
    {
        buttons |= _360_JOY_BUTTON_START | _360_JOY_BUTTON_B;
    }
    if (down(SDL_SCANCODE_UP))
    {
        buttons |= _360_JOY_BUTTON_DPAD_UP;
    }
    if (down(SDL_SCANCODE_DOWN))
    {
        buttons |= _360_JOY_BUTTON_DPAD_DOWN;
    }
    if (down(SDL_SCANCODE_LEFT))
    {
        buttons |= _360_JOY_BUTTON_DPAD_LEFT;
    }
    if (down(SDL_SCANCODE_RIGHT))
    {
        buttons |= _360_JOY_BUTTON_DPAD_RIGHT;
    }
    if (lmb)
    {
        buttons |= _360_JOY_BUTTON_RT;
    }
    if (rmb)
    {
        buttons |= _360_JOY_BUTTON_LT;
    }

    // Mouse wheel notches are momentary events, not held state - latch one
    // tick's worth of hotbar-scroll button per accumulated notch so
    // ButtonPressed() (edge-triggered) sees exactly one press per notch.
    int wheel = SDL3Input::ConsumeWheelSteps();
    if (wheel > 0)
    {
        buttons |= _360_JOY_BUTTON_RB;
    }
    else if (wheel < 0)
    {
        buttons |= _360_JOY_BUTTON_LB;
    }

    s_buttonsDown = buttons;

    s_stickLX = (down(SDL_SCANCODE_D) ? 1.0f : 0.0f) - (down(SDL_SCANCODE_A) ? 1.0f : 0.0f);
    s_stickLY = (down(SDL_SCANCODE_W) ? 1.0f : 0.0f) - (down(SDL_SCANCODE_S) ? 1.0f : 0.0f);

    float mouseDX = 0.0f, mouseDY = 0.0f;
    SDL_GetRelativeMouseState(&mouseDX, &mouseDY);
    if (Mouse::isGrabbed())
    {
        s_stickRX = Clamp(mouseDX * kMouseLookScale, -1.0f, 1.0f);
        s_stickRY = Clamp(-mouseDY * kMouseLookScale, -1.0f, 1.0f);
    }
    else
    {
        // Drain relative motion while a screen owns the pointer, but do not
        // rotate the player underneath menus.
        s_stickRX = s_stickRY = 0.0f;
    }
}

void C_4JInput::SetDeadzoneAndMovementRange(unsigned int, unsigned int)
{
}

void C_4JInput::SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction, unsigned int uiActionVal)
{
    if (ucMap >= kMaxMapStyles || ucAction >= kMaxActions)
    {
        return;
    }
    s_actionMap[ucMap][ucAction] = uiActionVal;
}

unsigned int C_4JInput::GetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction)
{
    if (ucMap >= kMaxMapStyles || ucAction >= kMaxActions)
    {
        return 0;
    }
    return s_actionMap[ucMap][ucAction];
}

void C_4JInput::SetJoypadMapVal(int iPad, unsigned char ucMap)
{
    if (iPad >= 0 && iPad < XUSER_MAX_COUNT)
    {
        s_padMapVal[iPad] = ucMap;
    }
}

unsigned char C_4JInput::GetJoypadMapVal(int iPad)
{
    return (iPad >= 0 && iPad < XUSER_MAX_COUNT) ? s_padMapVal[iPad] : 0;
}

void C_4JInput::SetJoypadSensitivity(int, float)
{
}

unsigned int C_4JInput::GetValue(int iPad, unsigned char ucAction, bool)
{
    return ButtonDown(iPad, ucAction) ? 255 : 0;
}

bool C_4JInput::ButtonPressed(int iPad, unsigned char ucAction)
{
    if (ucAction == 255)
    {
        return (s_buttonsDown & ~s_buttonsDownPrev) != 0;
    }
    unsigned int mask = GetGameJoypadMaps(GetJoypadMapVal(iPad), ucAction);
    return mask != 0 && (s_buttonsDown & mask) != 0 && (s_buttonsDownPrev & mask) == 0;
}

bool C_4JInput::ButtonReleased(int iPad, unsigned char ucAction)
{
    unsigned int mask = GetGameJoypadMaps(GetJoypadMapVal(iPad), ucAction);
    return mask != 0 && (s_buttonsDown & mask) == 0 && (s_buttonsDownPrev & mask) != 0;
}

bool C_4JInput::ButtonDown(int iPad, unsigned char ucAction)
{
    if (ucAction == 255)
    {
        return s_buttonsDown != 0;
    }
    unsigned int mask = GetGameJoypadMaps(GetJoypadMapVal(iPad), ucAction);
    return (s_buttonsDown & mask) != 0;
}

void C_4JInput::SetJoypadStickAxisMap(int, unsigned int, unsigned int)
{
}

void C_4JInput::SetJoypadStickTriggerMap(int, unsigned int, unsigned int)
{
}

void C_4JInput::SetKeyRepeatRate(float, float)
{
}

void C_4JInput::SetDebugSequence(const char *, int (*)(LPVOID), LPVOID)
{
}

FLOAT C_4JInput::GetIdleSeconds(int)
{
    return 0.0f;
}

bool C_4JInput::IsPadConnected(int iPad)
{
    // Keyboard + mouse is always "pad 0"; no other local pads exist.
    return iPad == 0;
}

float C_4JInput::GetJoypadStick_LX(int, bool)
{
    return s_stickLX;
}

float C_4JInput::GetJoypadStick_LY(int, bool)
{
    return s_stickLY;
}

float C_4JInput::GetJoypadStick_RX(int, bool)
{
    return s_stickRX;
}

float C_4JInput::GetJoypadStick_RY(int, bool)
{
    return s_stickRY;
}

unsigned char C_4JInput::GetJoypadLTrigger(int, bool)
{
    return (s_buttonsDown & _360_JOY_BUTTON_LT) ? 255 : 0;
}

unsigned char C_4JInput::GetJoypadRTrigger(int, bool)
{
    return (s_buttonsDown & _360_JOY_BUTTON_RT) ? 255 : 0;
}

void C_4JInput::SetMenuDisplayed(int, bool)
{
}

EKeyboardResult C_4JInput::RequestKeyboard(LPCWSTR, LPCWSTR, DWORD, UINT, int (*)(LPVOID, const bool), LPVOID, C_4JInput::EKeyboardMode)
{
    return EKeyboard_Cancelled;
}

void C_4JInput::GetText(uint16_t *)
{
}

bool C_4JInput::VerifyStrings(WCHAR **, int, int (*)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID)
{
    return false;
}

void C_4JInput::CancelQueuedVerifyStrings(int (*)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID)
{
}

void C_4JInput::CancelAllVerifyInProgress(void)
{
}
