#include <iostream>

#include <SDL3/SDL.h>

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "Error initializing video\n";
        return 1;
    }

    auto* window = SDL_CreateWindow("Hello", 1280, 720, 0);

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