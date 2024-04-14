#include <iostream>

#include <cassert>
#include <SL/Lua.hpp>
#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "Error initializing video\n";
        return 1;
    }

    const auto [title, width, height] = []()
    {
        SL::Runtime runtime(SOURCE_DIR "/window.lua");
        const auto res = runtime.getGlobal<SL::Table>("WindowOptions");
        assert(res);

        const auto& options = *res;
        const auto string = options.get<SL::String>("title");
        const auto w = options.get<SL::Table>("size").get<SL::Number>("w");
        const auto h = options.get<SL::Table>("size").get<SL::Number>("h");
        return std::tuple(string, w, h);
    }();

    auto* window = SDL_CreateWindow(title.c_str(), width, height, 0);

    bool close = false;
    while (!close)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                close = true;
        }

        SDL_UpdateWindowSurface(window);
    }
}