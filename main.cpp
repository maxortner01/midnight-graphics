#include <midnight/midnight.hpp>

#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

int main()
{
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    Pipeline pipeline;
    pipeline.addShader(SOURCE_DIR "/shaders/vertex.glsl",   ShaderType::Vertex);
    pipeline.addShader(SOURCE_DIR "/shaders/fragment.glsl", ShaderType::Fragment);
    pipeline.build();

    // Main loop
    while (!window.shouldClose())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                window.close();
        }

        auto frame = window.startFrame();
        window.testDisplay(frame);
        window.endFrame(frame);
    }
}
