#include <midnight/midnight.hpp>

#include "game/MeshGeneration.hpp"

#include <SDL3/SDL.h>

#include <set>
#include <stack>
#include <thread>
#include <atomic>

#include <SimplexNoise.h>

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

struct Chunk
{
    Chunk(const mn::Math::Vec3i& chunk_index) :
        location(chunk_index)
    {   }

    const mn::Math::Vec3i location;
    std::shared_ptr<mn::Graphics::Model> model;
};

int main()
{
    using namespace mn;
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    auto pipeline = PipelineBuilder::fromLua(SOURCE_DIR, "/pipelines/main.lua")
        .setDescriptorSize(sizeof(Uniform))
        .build();

    std::mutex chunk_mutex, generate_mutex;
    std::vector<std::shared_ptr<Chunk>> chunks;
    std::vector<std::shared_ptr<Chunk>> generate_chunks;

    std::atomic<bool> done(false);
	const auto thread_func = [&]()
    {
        while (!done)
        {
            if (!generate_chunks.size()) continue;

            generate_mutex.lock();
            auto chunk = generate_chunks.back();
            generate_chunks.pop_back();
            generate_mutex.unlock();

            chunk->model = Game::generate_mesh(chunk->location);

            chunk_mutex.lock();
            chunks.push_back(chunk);
            chunk_mutex.unlock();
        }
    };

	const auto GENERATOR_THREADS = 2U;
	const auto threads = [GENERATOR_THREADS, &thread_func]()
	{
		std::vector<std::unique_ptr<std::thread>> threads(GENERATOR_THREADS);
		for (auto& thread : threads)
			thread = std::make_unique<std::thread>(thread_func);
		return threads;
	}();

    auto player_pos = Math::Vec3f({ 0, 9, 0 });
	auto player_rot = Math::Vec3<Math::Angle>({ Math::Angle::radians(0), Math::Angle::radians(0), Math::Angle::radians(0) });

    std::vector<Math::Vec3i> radius;
	for (int32_t z = -1; z <= 1; z++)
		for (int32_t y = -1; y <= 1; y++)
			for (int32_t x = -1; x <= 1; x++)
				radius.push_back(Math::Vec3i({ x, y, z }));

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
    while (!window.shouldClose() && !done)
    {
		Math::Vec2f mouse_movement;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                window.close();

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
				{
					done = true;
				}

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
			(int32_t)(Math::x(player_pos) / Game::CHUNK_SIZE), 
			(int32_t)(Math::y(player_pos) / Game::CHUNK_SIZE),
			(int32_t)(Math::z(player_pos) / Game::CHUNK_SIZE) 
		});
        
        std::vector<std::shared_ptr<Chunk>> new_chunks;

        generate_mutex.lock();
        for (const auto& r : radius)
        {
            const auto pos = player_chunk + r;
            // check if chunk in chunks
            bool found = false;
            for (const auto& chunk : chunks)
                if (Math::x(chunk->location) == Math::x(pos) && 
					Math::y(chunk->location) == Math::y(pos) &&
					Math::z(chunk->location) == Math::z(pos))
                {
                    found = true;
                    break;
                }

            if (!found)
			{
                for (const auto& chunk : generate_chunks)
                    if (Math::x(chunk->location) == Math::x(pos) && 
						Math::y(chunk->location) == Math::y(pos) &&
						Math::z(chunk->location) == Math::z(pos))
                    {
                        found = true;
                        break;
                    }
			}

            if (!found) 
                generate_chunks.insert(generate_chunks.begin(), std::make_shared<Chunk>(pos));
        }

		std::sort(generate_chunks.begin(), generate_chunks.end(), [player_chunk](const auto& a, const auto& b) { return Math::length(b->location - player_chunk) < Math::length(a->location - player_chunk); });
		std::erase_if(generate_chunks, [player_chunk](const auto& a) { return Math::length(a->location - player_chunk) >= 3; });

        generate_mutex.unlock();
        
        auto& uniform = pipeline.descriptorData<Uniform>(0, 0);
        uniform.view = Math::translation(player_pos * -1.f) * Math::rotation<float>(player_rot);
		uniform.player_pos = player_pos;

		const auto forward = Math::Vec3f({
			sinf(Math::y(player_rot).asRadians()), 0.f, cosf(Math::y(player_rot).asRadians())
		});

        auto frame = window.startFrame();
        frame.clear({ 0.f, 0.f, 0.f });

        chunk_mutex.lock();
		std::erase_if(chunks, [player_chunk](const auto& a) { return Math::length(a->location - player_chunk) >= 3; });
        frame.startRender();
		{
			std::vector<std::shared_ptr<Chunk>> culled;
			for (const auto& chunk : chunks)
			{
				const auto diff = (Math::Vec3f)chunk->location - (Math::Vec3f)player_chunk;

				const auto dir = Math::inner(forward, diff);
				if (dir > 0 || Math::length(diff) >= 2) continue;
				culled.push_back(chunk);
			}

			for (const auto& chunk : culled)
            	frame.draw(pipeline, chunk->model);
		}
        frame.endRender();
        chunk_mutex.unlock();

        window.endFrame(frame);
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

    done = true;
    for (const auto& g : threads)
		g->join();
}
