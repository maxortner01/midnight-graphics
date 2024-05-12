#include "Engine.hpp"
#include "Components.hpp"

#include <SDL3/SDL.h>

#include <set>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

namespace Game
{

void Engine::start(const std::string& location, std::function<Scene*()> construct_scene)
{
    using namespace mn;

    // Construct the window
    auto window = Graphics::Window::fromLuaScript(location);
    const auto aspect = static_cast<float>(Math::x(window.size())) / static_cast<float>(Math::y(window.size()));

    // Build the pipeline
    auto pipeline = Graphics::PipelineBuilder::fromLua(SOURCE_DIR, "/pipelines/main.lua")
        .setPushConstantObject<Constants>()
        .build();

    // Set up the basic buffers
    FundBuffers buffers;
    buffers.frame_info.resize(1);
    buffers.models.resize(1);
    buffers.models[0] = Math::identity<4, float>();

    scene = std::unique_ptr<Scene>(construct_scene());

    std::set<SDL_Scancode> keys;
    bool constrained = true;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    auto now = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose() && !scene->done())
    {
		Math::Vec2f mouse_movement;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                window.close();
                scene->finish();
            }

            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (!keys.count(event.key.keysym.scancode))
                    keys.insert(event.key.keysym.scancode);

                if (event.key.keysym.scancode == SDL_SCANCODE_1)
                {
                    constrained = !constrained;
                    SDL_SetRelativeMouseMode(constrained);
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    scene->finish();

            }
            if (event.type == SDL_EVENT_KEY_UP)
            {
                if (keys.count(event.key.keysym.scancode))
                    keys.erase(event.key.keysym.scancode);
            }

			if (event.type == SDL_EVENT_MOUSE_MOTION)
			{
				Math::x(mouse_movement) = event.motion.xrel * 0.1f;
				Math::y(mouse_movement) = event.motion.yrel * 0.1f;
			}
        }

        window.finishWork();

        const auto new_now = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::chrono::seconds::period> dt = new_now - now;
        now = new_now;

        scene->update(dt.count());

        // 1) Get entity with camera and transform components, construct frame_info data
        auto& world = scene->world;
        const auto cameras = world.count<Components::Camera>();
        MIDNIGHT_ASSERT(cameras == 1, (cameras ? "Too many cameras" : "Must have one camera"));
        world.each([&](const Components::Camera& camera, Components::Transform& transform)
        {
            // Basic camera controls... These should go somewhere else
            const auto forward = Math::Vec3f({
                sinf(Math::y(transform.rotation).asRadians()), 0.f, cosf(Math::y(transform.rotation).asRadians())
            });

            const auto right = Math::Vec3f({
                cosf(Math::y(transform.rotation).asRadians() * -1.f), 0.f, sinf(Math::y(transform.rotation).asRadians() * -1.f)
            });

            // Basic rotation
            Math::x(transform.rotation) -= Math::Angle::radians(Math::y(mouse_movement) * dt.count());
            Math::y(transform.rotation) -= Math::Angle::radians(Math::x(mouse_movement) * dt.count());

            // Simple FPS movement
            const auto velocity = (keys.count(SDL_SCANCODE_LSHIFT) ? 5.f : 2.5f);

            if (keys.count(SDL_SCANCODE_SPACE))
                Math::y(transform.position) += velocity * dt.count();
            if (keys.count(SDL_SCANCODE_Z))
                Math::y(transform.position) -= velocity * dt.count();

            if (keys.count(SDL_SCANCODE_W))
                transform.position += forward * dt.count() * -1.f * velocity;
            if (keys.count(SDL_SCANCODE_S))
                transform.position += forward * dt.count() * velocity;

            if (keys.count(SDL_SCANCODE_A))
                transform.position += right * dt.count() * -1.f * velocity;
            if (keys.count(SDL_SCANCODE_D))
                transform.position += right * dt.count() * velocity;

            auto& data = buffers.frame_info[0];
            data.proj = Math::perspective(aspect, camera.FOV, { camera.near, camera.far });
            data.view = Math::translation(transform.position * -1.f) * Math::rotation<float>(transform.rotation);
            data.player_pos = transform.position;
        });

        // 2) Count entities with mesh and transform components, make sure buffer.size() == this count + 1
        uint32_t transform_meshes{0};
        world.each([&](const Components::Transform& transform, const Components::Mesh& mesh)
        {
            transform_meshes++;
        });
        if (buffers.models.size() != transform_meshes + 1)
            buffers.models.resize(transform_meshes + 1);

        Constants constants;
        constants.buffer = buffers.models.getAddress();
        constants.frame_info = buffers.frame_info.getAddress();
        constants.index = 0;

        window.runFrame([&](Graphics::RenderFrame& frame)
        {
            frame.clear({ 0.f, 0.f, 0.f });

			frame.startRender();

            // Go through all entities that have mesh components
            uint32_t current_index = 1;
            world.each([&](flecs::entity e, const Components::Mesh& mesh)
            {
                // Here we can cull based off the mesh information... if the frustum culling test fails then return

                // if entity has a transform component, set constants.index = current_index++ and construct model matrix at that index
                if (e.has<Components::Transform>())
                {
                    constants.index = current_index++;
                    buffers.models[constants.index] = Components::makeModelMatrix(*e.get<Components::Transform>());
                }
                // otherwise set constant.index = 0
                else
                    constants.index = 0;

                // setPushConstants
                frame.setPushConstant(pipeline, constants);
                
                // frame.draw
                frame.draw(pipeline, mesh.model);
                
            });

            frame.endRender();
        });
    }

    window.finishWork();
    scene->destroy();
}

}