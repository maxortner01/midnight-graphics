#include <midnight/midnight.hpp>

#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

int main()
{
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    Shader shader(SOURCE_DIR "/shaders/vertex.glsl", ShaderType::Vertex);

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
