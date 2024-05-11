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

struct Constants
{
    mn::Math::Mat4<float> proj, view, model;
	mn::Math::Vec3f player_pos;
	mn::Graphics::Buffer::gpu_addr buffer;
};

int main()
{
    using namespace mn;
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

	// maube instead of here we instead specify the size and count inside the actual
	// descriptor set, that way we can build/rebuild the buffers if needed on the fly

	// We need to rewrite pipeline.cpp
	// Let's take a step back from shader reflection and have the user specify the information
	// Firstly, 
    auto pipeline = PipelineBuilder::fromLua(SOURCE_DIR, "/pipelines/main.lua")
		.setPushConstantObject<Constants>()
        .build();

	Buffer data;
	data.allocateBytes(sizeof(float));

	Model cube = Model::fromLua(SOURCE_DIR "/models/cube.lua");

	const auto& world = Game::World::get();
	world->loadFromLua(SOURCE_DIR "/world.lua");

    auto player_pos = Math::Vec3f({ 0, 9, 0 });
	auto player_rot = Math::Vec3<Math::Angle>({ Math::Angle::radians(0), Math::Angle::radians(0), Math::Angle::radians(0) });

    const auto aspect = static_cast<float>(Math::x(window.size())) / static_cast<float>(Math::y(window.size()));
	const auto set_view = [&](const auto& frame, const auto& pipeline, Math::Vec3f scale, Math::Vec3f position)
	{
		Constants c;
        //auto& uniform = pipeline.template descriptorData<Uniform>(0, 0);
        c.proj  = Math::perspective(aspect, Math::Angle::degrees(75), { 0.1, 1000.0 });
		c.model = Math::scale(scale) * Math::translation(position);
        c.view  = Math::translation(player_pos * -1.f) * Math::rotation<float>(player_rot);
		c.player_pos = player_pos;
		c.buffer = data.getAddress();
		frame.setPushConstant(pipeline, c);
	};	

	// we want to be able to do multiple uniform bindings
	// we want to be able to stage buffer data so it's not mapped

    uint32_t frameCount = 0;
    double rotation = 0.0;

	std::set<SDL_Scancode> keys;
	bool constrained = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// cool... how do we do mesh independent positions?
	// looks like we need a way of allocating a variable amount of descriptor sets
	// So, ideally we can infer from a shader what kinds of descriptor sets there are
	// Then we want to be able to specify things like, hey make only one of binding 1
	// but 20 of binding 2. Then, we can bind the correct ones(?)
	// Or maybe we just have the user handle descriptor sets? Need to flesh this out ASAP
	// Should be able to specify count and bytesize per binding
	// then we can choose the binding indexes we want

	// next up: basic collisions (fcl) and physics
	auto cube_pos = Math::Vec3f({ 0.f, 9.f, -1.f });
	auto cube_vel = Math::Vec3f({ 0.f, 0.f, -2.f });

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

				set_view(frame, pipeline, Math::Vec3f{ 1.f,   1.f,   1.f    }, Math::Vec3f{0.f, 0.f, 0.f});
				for (const auto& chunk : chunks)
					frame.draw(pipeline, chunk->model);

				set_view(frame, pipeline, Math::Vec3f{ 0.25f, 0.25f, 0.25f, }, cube_pos);
				frame.draw(pipeline, cube); // need to be able to specify binding 0, index 0 && binding 1, index 1
					
				frame.endRender();
			});
		});
		
		const auto forward = Math::Vec3f({
			sinf(Math::y(player_rot).asRadians()), 0.f, cosf(Math::y(player_rot).asRadians())
		});
		
        const auto new_now = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::chrono::seconds::period> dt = new_now - now;
        window.setTitle((std::stringstream() << std::fixed << std::setprecision(2) << (1.0 / dt.count()) << " fps").str());
        now = new_now;

		/* Collision checking */
		// For every vertex, check if it's inside the world
		Math::Vec3f normal;
		const auto model = Math::scale(Math::Vec3f{ 0.25f, 0.25f, 0.25f, }) * Math::translation(cube_pos);
		for (const auto& vertex : cube.vertices())
		{
			const auto t_pos = [&]()
			{	
				const auto pos4 = Math::Vec4f({ Math::x(vertex.position), Math::y(vertex.position), Math::z(vertex.position), 1.f });	
				const auto t_pos4 = model * pos4;
				return Math::Vec3f({ Math::x(t_pos4), Math::y(t_pos4), Math::z(t_pos4) });
			}();

			//std::cout << Math::x(t_pos) << ", " << Math::y(t_pos) << ", " << Math::z(t_pos) << "\n";
			//std::cout << Game::World::inside(t_pos) << "\n";
			if (Game::World::inside(t_pos))
				normal += Game::World::getGradient(t_pos);
		}

		if (Math::length(normal))
		{
			cube_vel = Math::reflect(cube_vel, Math::normalized(normal));
			cube_pos += cube_vel * 0.1f;
		}

		cube_pos += cube_vel * dt.count();

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
