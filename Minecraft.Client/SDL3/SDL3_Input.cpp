#include "SDL3_Input.h"
#include "../stdafx.h"

namespace
{
int s_wheelSteps = 0;

int MapMouseButton(Uint8 sdlButton)
{
    // SDL buttons are 1-based (1=left, 2=middle, 3=right); the rest of the
    // codebase uses the LWJGL/Java convention (0=left, 1=right, 2=middle).
    switch (sdlButton)
    {
    case SDL_BUTTON_LEFT:
        return 0;
    case SDL_BUTTON_RIGHT:
        return 1;
    case SDL_BUTTON_MIDDLE:
        return 2;
    default:
        return -1;
    }
}

int WindowHeight(SDL_Window *window)
{
    if (!window)
    {
        return 0;
    }
    int w = 0, h = 0;
    // SDL mouse positions are in logical window coordinates, not drawable
    // pixels. Using the latter breaks Y inversion on HiDPI displays.
    SDL_GetWindowSize(window, &w, &h);
    return h;
}
} // namespace

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------
void Keyboard::enableRepeatEvents(bool enabled)
{
    s_repeatEventsEnabled = enabled;
    if (!s_window)
    {
        return;
    }
    if (enabled)
    {
        SDL_StartTextInput(s_window);
    }
    else
    {
        SDL_StopTextInput(s_window);
    }
}

bool Keyboard::next()
{
    if (s_queue.empty())
    {
        return false;
    }
    Event e = s_queue.front();
    s_queue.pop_front();
    s_curKey = e.key;
    s_curChar = e.ch;
    s_curDown = e.down;
    return true;
}

void Keyboard::pushKeyEvent(int key, bool down)
{
    if (s_queue.size() > 256)
    {
        s_queue.pop_front();
    }
    s_queue.push_back({key, down, 0});
}

void Keyboard::pushCharEvent(wchar_t ch)
{
    if (s_queue.size() > 256)
    {
        s_queue.pop_front();
    }
    s_queue.push_back({KEY_NONE, true, ch});
}

SDL_Scancode Keyboard::ToScancode(int key)
{
    if (key >= KEY_A && key <= KEY_Z)
    {
        return (SDL_Scancode)(SDL_SCANCODE_A + (key - KEY_A));
    }
    switch (key)
    {
    case KEY_SPACE:
        return SDL_SCANCODE_SPACE;
    case KEY_LSHIFT:
        return SDL_SCANCODE_LSHIFT;
    case KEY_ESCAPE:
        return SDL_SCANCODE_ESCAPE;
    case KEY_BACK:
        return SDL_SCANCODE_BACKSPACE;
    case KEY_RETURN:
        return SDL_SCANCODE_RETURN;
    case KEY_RSHIFT:
        return SDL_SCANCODE_RSHIFT;
    case KEY_UP:
        return SDL_SCANCODE_UP;
    case KEY_DOWN:
        return SDL_SCANCODE_DOWN;
    case KEY_TAB:
        return SDL_SCANCODE_TAB;
    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}

int Keyboard::FromScancode(SDL_Scancode sc)
{
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
    {
        return KEY_A + (sc - SDL_SCANCODE_A);
    }
    switch (sc)
    {
    case SDL_SCANCODE_SPACE:
        return KEY_SPACE;
    case SDL_SCANCODE_LSHIFT:
        return KEY_LSHIFT;
    case SDL_SCANCODE_ESCAPE:
        return KEY_ESCAPE;
    case SDL_SCANCODE_BACKSPACE:
        return KEY_BACK;
    case SDL_SCANCODE_RETURN:
        return KEY_RETURN;
    case SDL_SCANCODE_RSHIFT:
        return KEY_RSHIFT;
    case SDL_SCANCODE_UP:
        return KEY_UP;
    case SDL_SCANCODE_DOWN:
        return KEY_DOWN;
    case SDL_SCANCODE_TAB:
        return KEY_TAB;
    default:
        return KEY_NONE;
    }
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------
int Mouse::getX()
{
    float x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    return (int)x;
}

int Mouse::getY()
{
    float x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    return WindowHeight(s_window) - (int)y;
}

bool Mouse::next()
{
    if (s_queue.empty())
    {
        return false;
    }
    Event e = s_queue.front();
    s_queue.pop_front();
    s_curX = e.x;
    s_curY = e.y;
    s_curDX = e.dx;
    s_curDY = e.dy;
    s_curButton = e.button;
    s_curButtonState = e.buttonState;
    return true;
}

void Mouse::pushMotionEvent(int x, int y, int dx, int dy)
{
    if (s_queue.size() > 256)
    {
        s_queue.pop_front();
    }
    int flippedY = WindowHeight(s_window) - y;
    bool anyButtonDown = SDL_GetMouseState(nullptr, nullptr) != 0;
    s_queue.push_back({x, flippedY, dx, -dy, -1, anyButtonDown});
}

void Mouse::pushButtonEvent(int button, bool down, int x, int y)
{
    if (s_queue.size() > 256)
    {
        s_queue.pop_front();
    }
    int flippedY = WindowHeight(s_window) - y;
    s_queue.push_back({x, flippedY, 0, 0, button, down});
}

void Mouse::setGrabbed(bool grabbed)
{
    if (s_grabbed == grabbed)
    {
        return;
    }
    s_grabbed = grabbed;
    if (s_window)
    {
        SDL_SetWindowRelativeMouseMode(s_window, grabbed);

        // Discard the platform transition into or out of relative mode so it
        // cannot become a sudden camera turn on the next input tick.
        float ignoredX = 0, ignoredY = 0;
        SDL_GetRelativeMouseState(&ignoredX, &ignoredY);
    }
}

// ---------------------------------------------------------------------------
// SDL3Input
// ---------------------------------------------------------------------------
void SDL3Input::Init(SDL_Window *window)
{
    Keyboard::setWindow(window);
    Mouse::setWindow(window);
}

void SDL3Input::PumpEvent(const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
        {
            if (event.key.repeat && !Keyboard::isRepeatEnabled())
            {
                break;
            }
            int key = Keyboard::FromScancode(event.key.scancode);
            if (key != Keyboard::KEY_NONE)
            {
                Keyboard::pushKeyEvent(key, true);
            }
            break;
        }
    case SDL_EVENT_KEY_UP:
        {
            int key = Keyboard::FromScancode(event.key.scancode);
            if (key != Keyboard::KEY_NONE)
            {
                Keyboard::pushKeyEvent(key, false);
            }
            break;
        }
    case SDL_EVENT_TEXT_INPUT:
        {
            // event.text.text is UTF-8 (usually one codepoint per event); decode
            // it manually rather than pulling in a full text-conversion utility
            // for what's normally a single ASCII/Latin-1 chat character.
            const char *p = event.text.text;
            while (p && *p)
            {
                unsigned char c0 = (unsigned char)p[0];
                Uint32 cp;
                int consumed;
                if (c0 < 0x80)
                {
                    cp = c0;
                    consumed = 1;
                }
                else if ((c0 & 0xE0) == 0xC0 && p[1])
                {
                    cp = ((c0 & 0x1Fu) << 6) | ((unsigned char)p[1] & 0x3Fu);
                    consumed = 2;
                }
                else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2])
                {
                    cp = ((c0 & 0x0Fu) << 12) | (((unsigned char)p[1] & 0x3Fu) << 6) | ((unsigned char)p[2] & 0x3Fu);
                    consumed = 3;
                }
                else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3])
                {
                    cp = ((c0 & 0x07u) << 18) | (((unsigned char)p[1] & 0x3Fu) << 12) | (((unsigned char)p[2] & 0x3Fu) << 6) |
                         ((unsigned char)p[3] & 0x3Fu);
                    consumed = 4;
                }
                else
                {
                    cp = c0;
                    consumed = 1;
                }
                if (cp <= 0xFFFF)
                {
                    Keyboard::pushCharEvent((wchar_t)cp);
                }
                p += consumed;
            }
            break;
        }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            int button = MapMouseButton(event.button.button);
            if (button >= 0)
            {
                Mouse::pushButtonEvent(button, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN, (int)event.button.x, (int)event.button.y);
            }
            break;
        }
    case SDL_EVENT_MOUSE_MOTION:
        // Screen::mouseEvent() only dispatches clicks and releases; it has no
        // motion branch. Gameplay consumes SDL relative motion directly in
        // C_4JInput::Tick(), so queueing this event creates spurious
        // mouseReleased(-1) calls and repeated clicks while dragging UI.
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (event.wheel.y > 0)
        {
            ++s_wheelSteps;
        }
        else if (event.wheel.y < 0)
        {
            --s_wheelSteps;
        }
        break;
    default:
        break;
    }
}

int SDL3Input::ConsumeWheelSteps()
{
    int v = s_wheelSteps;
    s_wheelSteps = 0;
    return v;
}
