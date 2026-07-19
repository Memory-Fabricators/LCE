#pragma once

// Real SDL3-backed Keyboard/Mouse input for the desktop build.
//
// This replaces the generic no-op Keyboard/Mouse stubs in ../stubs.h (see the
// _SDL3 branch there) with a port of the original LWJGL-style event-queue API
// (next()/getEvent*()) that Screen::updateEvents()/mouseEvent()/keyboardEvent()
// already expect, plus the plain isKeyDown()/isButtonDown() polling API used
// pervasively elsewhere (shift-click detection, debug camera controls, UI drag
// and scroll handling, ...).
//
// SDL3Input::PumpEvent() is fed every SDL_Event pulled out of SDL_PollEvent by
// SDL3_Minecraft.cpp's main loop and turns it into Keyboard/Mouse queue
// entries. C_4JInput's SDL3 backend (4JLibs/src/4J_Input.cpp) reads the same
// SDL3 keyboard/mouse state directly to synthesize a virtual gamepad for
// gameplay (see that file for why).

#include "angle_wgpu.h"
#include <deque>
#include <string>

struct SDL_Window;
union SDL_Event;
class Keyboard
{
  public:
    static void create()
    {
    }
    static void destroy()
    {
    }

    static void setApp(WinitApp *app)
    {
        s_app = app;
    }
    static void setWindow(SDL_Window *window)
    {
        s_window = window;
    }

    static bool isKeyDown(int key)
    {
        if (s_app)
        {
            return winit_app_is_key_down(s_app, (uint32_t)key);
        }
        return false;
    }

    static std::wstring getKeyName(int key)
    {
        if (key >= KEY_A && key <= KEY_Z)
        {
            return std::wstring(1, (wchar_t)(L'A' + (key - KEY_A)));
        }
        switch (key)
        {
        case KEY_SPACE: return L"Space";
        case KEY_LSHIFT: return L"Left Shift";
        case KEY_ESCAPE: return L"Escape";
        case KEY_BACK: return L"Backspace";
        case KEY_RETURN: return L"Enter";
        case KEY_RSHIFT: return L"Right Shift";
        case KEY_UP: return L"Up";
        case KEY_DOWN: return L"Down";
        case KEY_TAB: return L"Tab";
        default: return L"Key";
        }
    }

    // Toggles OS text-composition input (SDL_StartTextInput/StopTextInput),
    // and whether OS key-repeat produces additional queue entries - mirrors
    // the original semantics where screens with text fields (ChatScreen,
    // CreateWorldScreen, ...) call this on init()/removed().
    static void enableRepeatEvents(bool enabled);
    static bool isRepeatEnabled()
    {
        return s_repeatEventsEnabled;
    }

    // LWJGL-style event queue, drained one entry at a time by
    // Screen::keyboardEvent() via next()/getEvent*().
    static bool next();
    static int getEventKey()
    {
        return s_curKey;
    }
    static wchar_t getEventCharacter()
    {
        return s_curChar;
    }
    static bool getEventKeyState()
    {
        return s_curDown;
    }

    // Fed by SDL3Input::PumpEvent().
    static void pushKeyEvent(int key, bool down);
    static void pushCharEvent(wchar_t ch);

    static int ToScancode(int key);
    static int FromScancode(int sc);

    static const int KEY_A = 0;
    static const int KEY_B = 1;
    static const int KEY_C = 2;
    static const int KEY_D = 3;
    static const int KEY_E = 4;
    static const int KEY_F = 5;
    static const int KEY_G = 6;
    static const int KEY_H = 7;
    static const int KEY_I = 8;
    static const int KEY_J = 9;
    static const int KEY_K = 10;
    static const int KEY_L = 11;
    static const int KEY_M = 12;
    static const int KEY_N = 13;
    static const int KEY_O = 14;
    static const int KEY_P = 15;
    static const int KEY_Q = 16;
    static const int KEY_R = 17;
    static const int KEY_S = 18;
    static const int KEY_T = 19;
    static const int KEY_U = 20;
    static const int KEY_V = 21;
    static const int KEY_W = 22;
    static const int KEY_X = 23;
    static const int KEY_Y = 24;
    static const int KEY_Z = 25;
    static const int KEY_SPACE = 26;
    static const int KEY_LSHIFT = 27;
    static const int KEY_ESCAPE = 28;
    static const int KEY_BACK = 29;
    static const int KEY_RETURN = 30;
    static const int KEY_RSHIFT = 31;
    static const int KEY_UP = 32;
    static const int KEY_DOWN = 33;
    static const int KEY_TAB = 34;
    static const int KEY_LEFT = 35;
    static const int KEY_RIGHT = 36;
    static const int KEY_F5 = 37;

    // Sentinel for physical keys with no Keyboard::KEY_* equivalent above -
    // still queued (as character-only or dispatch-and-ignore entries) so
    // Keyboard::next() faithfully drains everything SDL delivered.
    static const int KEY_NONE = -1;

  private:
    struct Event
    {
        int key;
        bool down;
        wchar_t ch;
    };
    static inline std::deque<Event> s_queue;
    static inline WinitApp *s_app = nullptr;
    static inline SDL_Window *s_window = nullptr;
    static inline bool s_repeatEventsEnabled = false;
    static inline int s_curKey = KEY_NONE;
    static inline wchar_t s_curChar = 0;
    static inline bool s_curDown = false;
};

class Mouse
{
  public:
    static void create()
    {
    }
    static void destroy()
    {
    }

    static void setApp(WinitApp *app)
    {
        s_app = app;
    }
    static void setWindow(SDL_Window *window)
    {
        s_window = window;
    }

    static int getX();
    static int getY();
    static bool isButtonDown(int button)
    {
        if (s_app)
        {
            return winit_app_is_button_down(s_app, (uint32_t)(button + 1));
        }
        return false;
    }

    // LWJGL-style event queue for Screen::mouseEvent().
    static bool next();
    static int getEventX()
    {
        return s_curX;
    }
    static int getEventY()
    {
        return s_curY;
    }
    static int getEventDX()
    {
        return s_curDX;
    }
    static int getEventDY()
    {
        return s_curDY;
    }
    static int getEventButton()
    {
        return s_curButton;
    }
    static bool getEventButtonState()
    {
        return s_curButtonState;
    }

    // Fed by SDL3Input::PumpEvent().
    static void pushMotionEvent(int x, int y, int dx, int dy);
    static void pushButtonEvent(int button, bool down, int x, int y);

    // Relative (FPS-style) capture, toggled by the main loop depending on
    // whether a Screen is up.
    static void setGrabbed(bool grabbed);
    static bool isGrabbed()
    {
        return s_grabbed;
    }

  private:
    struct Event
    {
        int x, y, dx, dy;
        int button;
        bool buttonState;
    };
    static inline std::deque<Event> s_queue;
    static inline WinitApp *s_app = nullptr;
    static inline SDL_Window *s_window = nullptr;
    static inline bool s_grabbed = false;
    static inline int s_curX = 0, s_curY = 0, s_curDX = 0, s_curDY = 0;
    static inline int s_curButton = -1;
    static inline bool s_curButtonState = false;
};

namespace SDL3Input
{
void Init(WinitApp *app);
void Init(SDL_Window *window);
WinitApp *GetApp();

// Called for every SDL_Event pulled from SDL_PollEvent() - feeds the
// Keyboard/Mouse event queues above and the mouse-wheel step counter.
void PumpEvent(const SDL_Event &event);
void PumpEvents(WinitApp *app);

// Consumes (and resets) the mouse-wheel steps accumulated since the last
// call. Used by C_4JInput::Tick() to drive hotbar scrolling.
int ConsumeWheelSteps();
} // namespace SDL3Input
