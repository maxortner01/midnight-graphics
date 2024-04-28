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

struct Uniform
{
    mn::Math::Mat4<float> proj, view, model;
};

int main()
{
    using namespace mn;
    using namespace mn::Graphics;


    auto window = Window::fromLuaScript("window.lua");

    auto pipeline = PipelineBuilder()
        .addShader(SOURCE_DIR "/shaders/vertex.glsl",   ShaderType::Vertex)
        .addShader(SOURCE_DIR "/shaders/fragment.glsl", ShaderType::Fragment)
        .setColorFormat(44)
        .setBackfaceCull(false)
        .setDescriptorSize(sizeof(Uniform)) // should infer this from the layout, or specify set byte size per layout
        .build();

    auto model = Model::fromLua(SOURCE_DIR "/models/plane.lua");

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

        auto& uniform = pipeline.descriptorData<Uniform>(0, 0);
        uniform.model = Math::rotation<float>({ Math::Angle::degrees(0), Math::Angle::degrees(0), Math::Angle::degrees(frameCount / 10.0) });

        auto frame = window.startFrame();
        frame.clear({ 1.f, (sin(frameCount / 100.f) + 1.f) * 0.5f, 0.f });
        
        frame.startRender();
        frame.draw(pipeline, model);
        frame.endRender();

        window.endFrame(frame);

        frameCount++;
    }

    window.finishWork();
}
