#include <midnight/midnight.hpp>

#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

int main()
{
    mn::Graphics::Window window("window.lua");

    // Main loop
    while (!window.shouldClose())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                window.close();
        }

        window.update();
    }
}
