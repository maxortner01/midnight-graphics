#include <midnight/midnight.hpp>

#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

struct Vertex
{
    struct 
    {
        float x, y, z;
    } position;
    struct 
    {
        float r, g, b, a;
    } color;
};

template<typename T>
struct Mat4
{
    T m[4][4] = {0};
};

struct Uniform
{
    Mat4<float> proj, view, model;
};

int main()
{
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    auto pipeline = PipelineBuilder()
        .addShader(SOURCE_DIR "/shaders/vertex.glsl",   ShaderType::Vertex)
        .addShader(SOURCE_DIR "/shaders/fragment.glsl", ShaderType::Fragment)
        .setColorFormat(44)
        .setBackfaceCull(false)
        .setDescriptorSize(sizeof(Uniform))
        .build();

    auto& uniform = pipeline.descriptorData<Uniform>(0, 0);
    uniform.model.m[0][0] = 0.5f;
    uniform.model.m[1][1] = 1;
    uniform.model.m[2][2] = 1;
    uniform.model.m[3][3] = 1;

    auto vertexBuffer = std::make_shared<TypeBuffer<Vertex>>();
    vertexBuffer->resize(3);
    vertexBuffer->at(0) = Vertex {
        .position = { 0, 0, 0 }
    };
    vertexBuffer->at(1) = Vertex {
        .position = { 1, 0, 0 }
    };
    vertexBuffer->at(2) = Vertex {
        .position = { 1, 1, 0 }
    };

    uint32_t frameCount = 0;

    // Main loop
    while (!window.shouldClose())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                window.close();
        }

        window.finishWork();
        vertexBuffer->at(1) = Vertex {
            .position = { cos(frameCount / 100.f), 0, 0 }
        };
        vertexBuffer->at(2) = Vertex {
            .position = { cos(frameCount / 100.f), sin(frameCount / 100.f), 0 }
        };

        auto frame = window.startFrame();
        frame.clear({ 1.f, (sin(frameCount / 100.f) + 1.f) * 0.5f, 0.f });
        
        frame.startRender();
        frame.draw(pipeline, vertexBuffer);
        frame.endRender();

        window.endFrame(frame);

        frameCount++;
    }

    window.finishWork();
}
