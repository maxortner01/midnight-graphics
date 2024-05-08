#include <midnight/midnight.hpp>

#include "game/World.hpp"

#include <SDL3/SDL.h>

#include <set>
#include <stack>
#include <thread>
#include <atomic>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

// GOALS
// I want to be able to generate vertex arrays in the GPU with compute shaders
//   As a first test, we can implement the meshing algorithm below in a compute shader
//   then.. marching cubes

// Basic controls. Want to be able to fly around

struct Uniform
{
    mn::Math::Mat4<float> proj, view, model;
	mn::Math::Vec3f player_pos;
};

int main()
{
    using namespace mn;
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    auto pipeline = PipelineBuilder::fromLua(SOURCE_DIR, "/pipelines/main.lua")
        .setDescriptorSize(sizeof(Uniform))
        .build();

	const auto& world = Game::World::get();
	world->loadFromLua(SOURCE_DIR "/world.lua");

    auto player_pos = Math::Vec3f({ 0, 9, 0 });
	auto player_rot = Math::Vec3<Math::Angle>({ Math::Angle::radians(0), Math::Angle::radians(0), Math::Angle::radians(0) });

    {
        const auto aspect = static_cast<float>(Math::x(window.size())) / static_cast<float>(Math::y(window.size()));
        auto& uniform = pipeline.descriptorData<Uniform>(0, 0);
        uniform.proj  = Math::perspective(aspect, Math::Angle::degrees(75), { 0.1, 1000.0 });
        uniform.model = Math::identity<4, float>();
    }

	// we want to be able to do multiple uniform bindings
	// we want to be able to stage buffer data so it's not mapped

    uint32_t frameCount = 0;
    double rotation = 0.0;

	std::set<SDL_Scancode> keys;
	bool constrained = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);

    // Main loop
    auto now = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose() && !world->done)
    {
		Math::Vec2f mouse_movement;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                world->done = true;

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
					world->done = true;

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

        const auto player_chunk = Math::Vec3i({ 
			(int32_t)(Math::x(player_pos) / world->chunkSize()), 
			(int32_t)(Math::y(player_pos) / world->chunkSize()),
			(int32_t)(Math::z(player_pos) / world->chunkSize()) 
		});

		world->checkChunks(player_chunk);

		window.runFrame([&](RenderFrame& frame)
		{
			frame.clear({ 0.f, 0.f, 0.f });

			world->useChunks([&](const auto& chunks)
			{
				frame.startRender();

				/*
				std::vector<std::shared_ptr<Game::Chunk>> culled;
				for (const auto& chunk : chunks)
				{
					const auto diff = (Math::Vec3f)chunk->location - (Math::Vec3f)player_chunk;

					const auto dir = Math::inner(forward, diff);
					if (dir > 0 || Math::length(diff) >= 2) continue;
					culled.push_back(chunk);
				}*/

				for (const auto& chunk : chunks)
					frame.draw(pipeline, chunk->model);
					
				frame.endRender();
			});
		});

        
        auto& uniform = pipeline.descriptorData<Uniform>(0, 0);
        uniform.view = Math::translation(player_pos * -1.f) * Math::rotation<float>(player_rot);
		uniform.player_pos = player_pos;

		const auto forward = Math::Vec3f({
			sinf(Math::y(player_rot).asRadians()), 0.f, cosf(Math::y(player_rot).asRadians())
		});
		
        const auto new_now = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::chrono::seconds::period> dt = new_now - now;
        window.setTitle((std::stringstream() << std::fixed << std::setprecision(2) << (1.0 / dt.count()) << " fps").str());
        now = new_now;

		Math::x(player_rot) -= Math::Angle::radians(Math::y(mouse_movement) * dt.count());
		Math::y(player_rot) -= Math::Angle::radians(Math::x(mouse_movement) * dt.count());

		const auto right = Math::Vec3f({
			cosf(Math::y(player_rot).asRadians() * -1.f), 0.f, sinf(Math::y(player_rot).asRadians() * -1.f)
		});

		const auto velocity = (keys.count(SDL_SCANCODE_LSHIFT) ? 5.f : 2.5f);

		if (keys.count(SDL_SCANCODE_SPACE))
		{
			Math::y(player_pos) += velocity * dt.count();
		}
		if (keys.count(SDL_SCANCODE_Z))
		{
			Math::y(player_pos) -= velocity * dt.count();
		}

		if (keys.count(SDL_SCANCODE_W))
			player_pos += forward * dt.count() * -1.f * velocity;
		if (keys.count(SDL_SCANCODE_S))
			player_pos += forward * dt.count() * velocity;

		if (keys.count(SDL_SCANCODE_A))
			player_pos += right * dt.count() * -1.f * velocity;
		if (keys.count(SDL_SCANCODE_D))
			player_pos += right * dt.count() * velocity;

        frameCount++;
    }

    window.finishWork();
	world->finish();
	Game::World::destroy();
}
