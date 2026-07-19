#include "SDL3_Input.h"
#include "../stdafx.h"

namespace
{
int s_wheelSteps = 0;
} // namespace

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------
void Keyboard::enableRepeatEvents(bool enabled)
{
    s_repeatEventsEnabled = enabled;
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

int Keyboard::ToScancode(int key)
{
    return key;
}

int Keyboard::FromScancode(int sc)
{
    return sc;
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------
int Mouse::getX()
{
    if (s_app)
    {
        double x = 0, y = 0;
        winit_app_get_mouse_pos(s_app, &x, &y);
        return (int)x;
    }
    return 0;
}

int Mouse::getY()
{
    if (s_app)
    {
        double x = 0, y = 0;
        uint32_t w = 0, h = 0;
        winit_app_get_mouse_pos(s_app, &x, &y);
        winit_app_get_size(s_app, &w, &h);
        return (int)h - (int)y;
    }
    return 0;
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
    uint32_t w = 0, h = 0;
    if (s_app)
    {
        winit_app_get_size(s_app, &w, &h);
    }
    int flippedY = (int)h - y;
    s_queue.push_back({x, flippedY, dx, -dy, -1, false});
}

void Mouse::pushButtonEvent(int button, bool down, int x, int y)
{
    if (s_queue.size() > 256)
    {
        s_queue.pop_front();
    }
    uint32_t w = 0, h = 0;
    if (s_app)
    {
        winit_app_get_size(s_app, &w, &h);
    }
    int flippedY = (int)h - y;
    s_queue.push_back({x, flippedY, 0, 0, button, down});
}

void Mouse::setGrabbed(bool grabbed)
{
    if (s_grabbed == grabbed)
    {
        return;
    }
    s_grabbed = grabbed;
    if (s_app)
    {
        winit_app_set_mouse_grab(s_app, grabbed);
        winit_app_set_cursor_visible(s_app, !grabbed);
        double ignoredX = 0, ignoredY = 0;
        winit_app_get_mouse_delta(s_app, &ignoredX, &ignoredY);
    }
}

// ---------------------------------------------------------------------------
// SDL3Input
// ---------------------------------------------------------------------------
static WinitApp *s_globalApp = nullptr;

void SDL3Input::Init(WinitApp *app)
{
    s_globalApp = app;
    Keyboard::setApp(app);
    Mouse::setApp(app);
}

void SDL3Input::Init(SDL_Window *)
{
}

WinitApp *SDL3Input::GetApp()
{
    return s_globalApp;
}

void SDL3Input::PumpEvents(WinitApp *app)
{
    if (app)
    {
        winit_app_poll_events(app);
    }
}
void SDL3Input::PumpEvent(const SDL_Event &)
{
}
int SDL3Input::ConsumeWheelSteps()
{
    if (s_globalApp)
    {
        return (int)winit_app_consume_wheel_delta(s_globalApp);
    }
    int steps = s_wheelSteps;
    s_wheelSteps = 0;
    return steps;
}
