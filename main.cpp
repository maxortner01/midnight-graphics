#include <midnight/midnight.hpp>

#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

struct Uniform
{
    mn::Math::Mat4<float> proj, view, model;
};

int main()
{
    using namespace mn;
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    auto pipeline = PipelineBuilder::fromLua(SOURCE_DIR, "/pipelines/main.lua")
        .setDescriptorSize(sizeof(Uniform))
        .build();

    auto model = Model::fromLua(SOURCE_DIR "/models/cube.lua");

    {
        auto& uniform = pipeline.descriptorData<Uniform>(0, 0);
        uniform.proj = Math::perspective(1280.0 / 720.0, Math::Angle::degrees(75), { 0.1, 100.0 });
    }

    uint32_t frameCount = 0;
    double rotation = 0.0;

    // Main loop
    auto now = std::chrono::high_resolution_clock::now();
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
        uniform.model = Math::rotation<float>({ 
            Math::Angle::degrees(rotation / 7.0), 
            Math::Angle::degrees(rotation / 2.0), 
            Math::Angle::degrees(rotation) 
        }) * Math::translation(Math::Vec3<double>{ 0.0, 0.0, -5.0 });

        auto frame = window.startFrame();
        frame.clear({ 1.f, (sin(frameCount / 100.f) + 1.f) * 0.5f, 0.f });
        
        frame.startRender();
        frame.draw(pipeline, model);
        frame.endRender();

        window.endFrame(frame);
        const auto new_now = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::chrono::seconds::period> dt = new_now - now;
        window.setTitle((std::stringstream() << std::fixed << std::setprecision(2) << (1.0 / dt.count()) << " fps").str());
        now = new_now;

        rotation += 85.0 * dt.count();

        frameCount++;
    }

    window.finishWork();
}
