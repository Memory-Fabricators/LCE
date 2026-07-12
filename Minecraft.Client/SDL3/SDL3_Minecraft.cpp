// SDL3 platform entry point.
// Mirrors Windows64_Minecraft.cpp's game loop structure.

#include "../stdafx.h"

#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/IntCache.h"
#include "../../Minecraft.World/OldChunkStorage.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/compression.h"
#include "../../Minecraft.World/net.minecraft.world.level.tile.h"
#include "../Common/Audio/SoundEngine.h"
#include "../MinecraftServer.h"
#include "../Options.h"
#include "../PauseScreen.h"
#include "../Tesselator.h"
#include "../Textures.h"
#include "../User.h"
#include "SDL3_App.h"
#include "SDL3_Input.h"
#include "SDL3_UIController.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <cstdlib>
#include <unistd.h>

#include "4JLibs/inc/4J_Render.h"

// Default (and, for now, only) keyboard+mouse control scheme. Mirrors
// Windows64_Minecraft.cpp's DefineActions()/MAP_STYLE_0 block: maps the
// abstract MINECRAFT_ACTION_*/ACTION_MENU_* actions the rest of the client
// already consumes onto the same _360_JOY_BUTTON_* tokens the SDL3 backend
// of C_4JInput (4JLibs/src/4J_Input.cpp) sets from real keys/mouse buttons
// each tick. There's no in-game control-remapping UI on this platform yet
// (Common/UI is unported), so MAP_STYLE_1/2 (southpaw/alternate schemes)
// aren't populated - GetJoypadMapVal() always returns 0.
static void DefineActions(void)
{
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_A, _360_JOY_BUTTON_A);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_B, _360_JOY_BUTTON_B);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_X, _360_JOY_BUTTON_X);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_Y, _360_JOY_BUTTON_Y);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OK, _360_JOY_BUTTON_A);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_CANCEL, _360_JOY_BUTTON_B);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_UP, _360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_DOWN, _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_LEFT, _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEUP, _360_JOY_BUTTON_LT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEDOWN, _360_JOY_BUTTON_RT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_RIGHT_SCROLL, _360_JOY_BUTTON_RB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_LEFT_SCROLL, _360_JOY_BUTTON_LB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAUSEMENU, _360_JOY_BUTTON_START);

    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_STICK_PRESS, _360_JOY_BUTTON_LTHUMB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_PRESS, _360_JOY_BUTTON_RTHUMB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_UP, _360_JOY_BUTTON_RSTICK_UP);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OTHER_STICK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);

    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_JUMP, _360_JOY_BUTTON_A);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_FORWARD, _360_JOY_BUTTON_LSTICK_UP);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_BACKWARD, _360_JOY_BUTTON_LSTICK_DOWN);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LEFT, _360_JOY_BUTTON_LSTICK_LEFT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RIGHT, _360_JOY_BUTTON_LSTICK_RIGHT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_LEFT, _360_JOY_BUTTON_RSTICK_LEFT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_RIGHT, _360_JOY_BUTTON_RSTICK_RIGHT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_UP, _360_JOY_BUTTON_RSTICK_UP);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LOOK_DOWN, _360_JOY_BUTTON_RSTICK_DOWN);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_USE, _360_JOY_BUTTON_LT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_ACTION, _360_JOY_BUTTON_RT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RIGHT_SCROLL, _360_JOY_BUTTON_RB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_LEFT_SCROLL, _360_JOY_BUTTON_LB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_INVENTORY, _360_JOY_BUTTON_Y);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_PAUSEMENU, _360_JOY_BUTTON_START);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DROP, _360_JOY_BUTTON_B);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_SNEAK_TOGGLE, _360_JOY_BUTTON_RTHUMB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_CRAFTING, _360_JOY_BUTTON_X);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_RENDER_THIRD_PERSON, _360_JOY_BUTTON_LTHUMB);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_GAME_INFO, _360_JOY_BUTTON_BACK);

    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_LEFT, _360_JOY_BUTTON_DPAD_LEFT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_RIGHT, _360_JOY_BUTTON_DPAD_RIGHT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_UP, _360_JOY_BUTTON_DPAD_UP);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, MINECRAFT_ACTION_DPAD_DOWN, _360_JOY_BUTTON_DPAD_DOWN);
}

// Tries to chdir() into an assets root, verifying first that it looks like
// one (i.e. it actually contains the media archive this game loads on
// startup - see CMinecraftApp's mediapath). Returns true once a chdir()
// actually lands somewhere that checks out, so ResolveAssetsDir() can try
// the next candidate on failure instead of silently running from whatever
// directory happened to work.
static bool TryChdirToAssetsDir(const char *dir)
{
    if (!dir || dir[0] == '\0')
    {
        return false;
    }

    char probePath[1024];
    SDL_snprintf(probePath, sizeof(probePath), "%s/Common/Media/MediaWindows64.arc", dir);
    if (SDL_GetPathInfo(probePath, NULL))
    {
        if (chdir(dir) == 0)
        {
            return true;
        }
        SDL_Log("Warning: found assets at '%s' but chdir() failed", dir);
    }
    return false;
}

// The game loads all its assets (Common/, the media archive, texture packs,
// colours.col, etc.) via paths relative to the current working directory,
// so this anchors the CWD to a known assets root rather than depending on
// where the binary happens to be launched from. Resolution order:
//
//  1. LCE_ASSETS_DIR (env var) - explicit override, always wins if set and
//     valid.
//  2. LCE_ASSETS_DIR_DEFAULT - the source-tree path baked in at build time
//     (see Minecraft.Client/meson.build). Works great for running the
//     freshly built binary in-place on the machine that built it, but is an
//     absolute path from that machine, so it silently stops applying the
//     moment the binary is copied/packaged elsewhere - it's verified to
//     exist before use rather than trusted blindly.
//  3. Paths relative to SDL_GetBasePath() (the directory the executable
//     actually lives in, resolved portably via /proc/self/exe and
//     equivalents - see SDL_GetBasePath() docs), trying both "next to the
//     binary" and "Minecraft.Client/ next to the binary" (the layout an
//     installed/packaged build is likely to use). This is what makes
//     relocated/installed builds - not just in-tree ones - find their
//     assets.
//  4. Give up and run from the current working directory, same as if none
//     of this existed, but with a clear warning instead of a silent later
//     failure to load the media archive.
static void ResolveAssetsDir(void)
{
    const char *envDir = SDL_getenv("LCE_ASSETS_DIR");
    if (TryChdirToAssetsDir(envDir))
    {
        return;
    }
    if (envDir && envDir[0] != '\0')
    {
        SDL_Log("Warning: LCE_ASSETS_DIR='%s' doesn't look like a valid assets "
                "root (no Common/Media/MediaWindows64.arc found); ignoring it",
                envDir);
    }

#ifdef LCE_ASSETS_DIR_DEFAULT
    if (TryChdirToAssetsDir(LCE_ASSETS_DIR_DEFAULT))
    {
        return;
    }
#endif

    const char *basePath = SDL_GetBasePath();
    if (basePath != NULL)
    {
        char candidate[1024];

        // Binary sitting directly in the assets root (e.g. a dev build run
        // in-place from Minecraft.Client/, or an install layout that copies
        // assets next to the executable).
        SDL_snprintf(candidate, sizeof(candidate), "%s", basePath);
        if (TryChdirToAssetsDir(candidate))
        {
            return;
        }

        // Binary sitting next to a Minecraft.Client/ directory (e.g. running
        // straight out of a build directory that mirrors the source tree
        // layout, such as this project's own meson build/ output).
        SDL_snprintf(candidate, sizeof(candidate), "%sMinecraft.Client", basePath);
        if (TryChdirToAssetsDir(candidate))
        {
            return;
        }
    }

    SDL_Log("Warning: could not locate an assets directory (checked "
            "LCE_ASSETS_DIR, the build-time default, and paths relative to "
            "the executable); running from the current working directory "
            "and hoping for the best. Set LCE_ASSETS_DIR to override.");
}

int main(int argc, char *argv[])
{
    ResolveAssetsDir();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    // Request a double-buffered 24-bit color depth setup
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Rendering goes through ANGLE/EGL directly (4J_Render.cpp), not SDL's own
    // GL context - SDL_WINDOW_OPENGL would make SDL attach its own (unused)
    // GL-backed layer to the window, which can end up as what's actually
    // presented instead of the Metal/EGL layer ANGLE renders into. Request the
    // native surface type each backend actually needs instead.
    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    SDL_Window *window = SDL_CreateWindow("Minecraft", 1280, 720, windowFlags);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Hand the window to the keyboard/mouse backend (Keyboard/Mouse's
    // isKeyDown/isButtonDown/event-queue implementations in SDL3_Input.cpp,
    // and the mouse-look relative-capture toggle below).
    SDL3Input::Init(window);

    // Initialise render manager (creates EGL context, etc.)
    RenderManager.Initialise(window);
    RenderManager.InitialiseContext();

    // Initialise thread-local storage (mirrors Windows64_Minecraft.cpp)
    Tesselator::CreateNewThreadStorage(1024 * 1024);
    AABB::CreateNewThreadStorage();
    Vec3::CreateNewThreadStorage();
    IntCache::CreateNewThreadStorage();
    Compression::CreateNewThreadStorage();
    OldChunkStorage::CreateNewThreadStorage();
    Tile::CreateNewThreadStorage();

    // Load media archive and string table - must happen before Minecraft::main(),
    // which pulls default game rules out of the archive (GameRuleManager::loadDefaultGameRules).
    app.loadMediaArchive();
    app.loadStringTable();

    // Bootstrap Minecraft singleton (this initialises Tile, etc.)
    Minecraft::main();
    Minecraft *pMinecraft = Minecraft::GetInstance();

    // Sets up s_pPlatformNetworkManager (CPlatformNetworkManagerStub on this
    // platform) - TemporaryCreateGameStart() dereferences it via
    // g_NetworkManager.FakeLocalPlayerJoined() further down.
    g_NetworkManager.Initialise();

    // Initialise game settings
    app.InitGameSettings();

    // Set default sound levels
    pMinecraft->options->set(Options::Option::MUSIC, 1.0f);
    pMinecraft->options->set(Options::Option::SOUND, 1.0f);

    // Kick off a game (flat world, creative) - texture packs are loaded by
    // this point, so nothing is blocking this anymore.
    app.TemporaryCreateGameStart();

    app.InitialiseTips();

    // Set the number of possible joypad layouts the user can switch between
    // and the number of actions, load the default keyboard+mouse control
    // scheme (see DefineActions() above), and pick pad 0's map style - the
    // only one populated, since there's no in-game remapping UI yet.
    InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
    DefineActions();
    InputManager.SetJoypadMapVal(0, 0);
    InputManager.SetKeyRepeatRate(0.3f, 0.2f);

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            SDL3Input::PumpEvent(event);

            if (event.type == SDL_EVENT_QUIT ||
                event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                running = false;
            }
        }

        // Mouse-look (relative capture) only while there's no menu/UI screen
        // up to click around in - matches every other platform's "gameplay
        // input only reaches the player while screen == NULL" convention
        // (see e.g. Minecraft::tick()'s screen == NULL gating).
        Mouse::setGrabbed(pMinecraft->screen == NULL && app.GetGameStarted());

        RenderManager.StartFrame();

        app.UpdateTime();
        InputManager.Tick();
        RenderManager.Tick();

        if (app.GetGameStarted())
        {
            pMinecraft->run_middle();
            app.SetAppPaused(false);
        }
        else
        {
            pMinecraft->soundEngine->tick(NULL, 0.0f);
            pMinecraft->textures->tick(true, false);
            IntCache::Reset();
        }

        pMinecraft->soundEngine->playMusicTick();

        // Mirrors Windows64_Minecraft.cpp: tick/render the UI controller before
        // presenting. Both are no-ops on this platform for now (Common/UI/Iggy
        // isn't ported yet) but keeping the call here matches the real frame
        // sequence so it starts doing something the moment that lands.
        ui.tick();
        ui.render();

        RenderManager.Present();

        ui.CheckMenuDisplayed();

        app.HandleXuiActions();

        Vec3::resetPool();
    }

    // Shutdown. The game spins up a fleet of worker threads (chunk rebuild,
    // server, save, connections, update) that, following the console origin of
    // this code, run for the whole process lifetime blocked on events with no
    // cancellation path - the console simply powered off. Returning from main()
    // here runs static/atexit destructors (e.g. the renderer's command-buffer
    // mutex) while those threads are still calling into that state, which is the
    // "mutex lock failed: Invalid argument" abort seen on window close. Until
    // each subsystem grows a real cooperative shutdown, terminate the process
    // immediately without static teardown so nothing races the live threads.
    // The OS reclaims the window, GL context and memory on exit.
    std::_Exit(0);
}
