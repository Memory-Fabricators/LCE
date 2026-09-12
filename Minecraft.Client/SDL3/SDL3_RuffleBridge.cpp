#include "SDL3_RuffleBridge.h"
#include "angle_wgpu.h"
#include "client_platform.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include "ruffle_bridge.h"

namespace
{
bool IsOk(RuffleBridgeStatus status)
{
    return status == RUFFLE_BRIDGE_OK;
}
}

void RuffleBridge::Initialise(WinitApp *app)
{
    m_app = app;
    if (app)
    {
        uint32_t w = 0, h = 0;
        winit_app_get_size(app, &w, &h);
        m_width = (int)w;
        m_height = (int)h;
    }
    else
    {
        m_width = 1280;
        m_height = 720;
    }
    m_lastTickSeconds = 0.0;

    const char *swfPath = getenv("LCE_RUFFLE_SWF");
    if (swfPath != nullptr && *swfPath != '\0')
    {
        std::ifstream file(swfPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            fprintf(stderr, "Ruffle bridge: failed to read SWF '%s'; disabling bridge\n", swfPath);
            m_disabled = true;
            return;
        }
        size_t swfSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> swfBuffer(swfSize);
        file.read(reinterpret_cast<char *>(swfBuffer.data()), swfSize);

        std::string path(swfPath);
        const size_t lastSlash = path.find_last_of("/\\");
        const std::string baseDir = (lastSlash == std::string::npos) ? "." : path.substr(0, lastSlash);

        std::vector<std::string> searchDirStrings;
        searchDirStrings.push_back(baseDir);
        const char *extraDir = getenv("LCE_RUFFLE_SWF_EXTRA_DIR");
        if (extraDir != nullptr && *extraDir != '\0')
        {
            searchDirStrings.emplace_back(extraDir);
        }
        std::vector<const char *> searchDirPtrs;
        for (const std::string &dir : searchDirStrings)
        {
            searchDirPtrs.push_back(dir.c_str());
        }

        char url[1024];
        snprintf(url, sizeof(url), "file:///%s", swfPath);
        const RuffleBridgeStatus status = ruffle_bridge_player_create(
            static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height),
            swfBuffer.data(), swfSize, url,
            searchDirPtrs.data(), searchDirPtrs.size(), &m_player);

        if (!IsOk(status))
        {
            fprintf(stderr, "Ruffle bridge: player creation failed (%d) for '%s'; disabling bridge\n", static_cast<int>(status), swfPath);
            m_player = nullptr;
            m_disabled = true;
        }
        return;
    }

    const RuffleBridgeStatus status = ruffle_bridge_renderer_create(
        static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), &m_renderer);
    if (!IsOk(status))
    {
        fprintf(stderr, "Ruffle bridge initialization failed (%d); disabling bridge\n", static_cast<int>(status));
        m_renderer = nullptr;
        m_disabled = true;
    }
}

void RuffleBridge::Initialise(SDL_Window *window)
{
    m_window = window;
    Initialise(static_cast<WinitApp *>(nullptr));
}
void RuffleBridge::Tick()
{
    if (m_disabled)
        return;

    int width = m_width;
    int height = m_height;
    if (m_app)
    {
        uint32_t w = 0, h = 0;
        winit_app_get_size(m_app, &w, &h);
        width = (int)w;
        height = (int)h;
    }
    const bool resized = (width != m_width || height != m_height);

    if (m_player != nullptr)
    {
        if (resized)
        {
            const RuffleBridgeStatus status = ruffle_bridge_player_resize(
                m_player, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            if (!IsOk(status))
            {
                fprintf(stderr, "Ruffle bridge: player resize failed (%d); disabling bridge\n", static_cast<int>(status));
                m_disabled = true;
                return;
            }
            m_width = width;
            m_height = height;
        }

        static const auto start_time = std::chrono::steady_clock::now();
        const auto now_duration = std::chrono::steady_clock::now() - start_time;
        const double now = std::chrono::duration<double>(now_duration).count();
        const double dt = (m_lastTickSeconds > 0.0) ? (now - m_lastTickSeconds) : 0.016;
        m_lastTickSeconds = now;

        RuffleBridgeStatus status = ruffle_bridge_player_tick(m_player, dt);
        if (IsOk(status))
        {
            status = ruffle_bridge_player_render(m_player);
        }
        if (!IsOk(status))
        {
            fprintf(stderr, "Ruffle bridge: player tick/render failed (%d); disabling bridge\n", static_cast<int>(status));
            m_disabled = true;
        }
        return;
    }

    if (m_renderer == nullptr)
        return;

    if (resized)
    {
        const RuffleBridgeStatus status = ruffle_bridge_renderer_resize(
            m_renderer, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        if (!IsOk(status))
        {
            fprintf(stderr, "Ruffle bridge resize failed (%d); disabling bridge\n", static_cast<int>(status));
            m_disabled = true;
            return;
        }
        m_width = width;
        m_height = height;
    }

    const char *probeFlag = getenv("LCE_RUFFLE_BRIDGE_PROBE");
    if (probeFlag != nullptr && *probeFlag != '\0')
    {
        const RuffleBridgeStatus status = ruffle_bridge_renderer_render_probe(m_renderer);
        if (!IsOk(status))
        {
            fprintf(stderr, "Ruffle bridge render failed (%d); disabling bridge\n", static_cast<int>(status));
            m_disabled = true;
        }
    }
}

void RuffleBridge::Shutdown()
{
    if (m_player != nullptr)
    {
        ruffle_bridge_player_destroy(m_player);
        m_player = nullptr;
    }
    if (m_renderer != nullptr)
    {
        ruffle_bridge_renderer_destroy(m_renderer);
        m_renderer = nullptr;
    }
}
