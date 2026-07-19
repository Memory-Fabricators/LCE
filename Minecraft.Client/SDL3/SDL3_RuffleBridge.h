#pragma once

struct WinitApp;
struct SDL_Window;
struct RuffleBridgeRenderer;
struct RuffleBridgePlayer;

class RuffleBridge
{
public:
    void Initialise(WinitApp *app);
    void Initialise(SDL_Window *window);
    void Tick();
    void Shutdown();

private:
    WinitApp *m_app = nullptr;
    SDL_Window *m_window = nullptr;
    RuffleBridgeRenderer *m_renderer = nullptr;
    RuffleBridgePlayer *m_player = nullptr;
    double m_lastTickSeconds = 0.0;
    int m_width = 0;
    int m_height = 0;
    bool m_disabled = false;
};
