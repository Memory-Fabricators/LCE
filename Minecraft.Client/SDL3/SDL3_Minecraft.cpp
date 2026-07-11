// SDL3 platform entry point.
//
// M1 scope: bring up an SDL3 window + the Dawn/WebGPU device via
// RenderManager, and pump a bare event/render loop so a solid clear color is
// visible on screen. Wiring this into the real game tick/update loop is
// follow-up work for later milestones once DrawVertices/CBuff* are real.

#include "../stdafx.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "4JLibs/inc/4J_Render.h"

int main(int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Minecraft", 1280, 720, SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    RenderManager.Initialise(window);
    RenderManager.InitialiseContext();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED))
            {
                running = false;
            }
        }

        RenderManager.StartFrame();
        RenderManager.Clear(CLEAR_COLOUR_FLAG | CLEAR_DEPTH_FLAG);
        RenderManager.Present();
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
