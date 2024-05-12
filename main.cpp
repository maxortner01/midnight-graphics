#include <midnight/midnight.hpp>

#include "game/Components.hpp"
#include "game/World.hpp"
#include "game/Engine.hpp"

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

// Shoudl be able to render without indices (wasted memory)
// need to implement frustum culling and prefer to generate chunks in FOV
// need to see where the bottleneck is

// possibly some mesh optomizations

struct Scene : Game::Scene
{
	flecs::entity box, camera;

	Scene() :
		_done(false)
	{
		using namespace Game;
		using namespace mn;

		World::get()->loadFromLua(SOURCE_DIR "/world.lua");
		
		camera = world.entity();
		camera.set<Components::Camera>(Components::Camera { .far = 1000, .near = 0.01, .FOV = Math::Angle::degrees(75.f) });
		camera.set<Components::Transform>({ });

		box = world.entity();
		box.set<Components::Mesh>(Components::Mesh { .model = std::make_shared<Graphics::Model>(Graphics::Model::fromLua(SOURCE_DIR "/models/cube.lua")) });
		box.set<Components::Transform>(Components::Transform { .scale = Math::Vec3f{ 1.f, 1.f, 1.f } });
	}

	void update(double dt) override
	{
		using namespace Game;
		using namespace mn;

		Math::z(box.get_mut<Components::Transform>()->position) -= 0.45f * dt;

		auto game_world = Game::World::get();
		const auto& player_pos = camera.get<Components::Transform>()->position;
        const auto player_chunk = Math::Vec3i({ 
			(int32_t)(Math::x(player_pos) / game_world->chunkSize()), 
			(int32_t)(Math::y(player_pos) / game_world->chunkSize()),
			(int32_t)(Math::z(player_pos) / game_world->chunkSize()) 
		});

		game_world->checkChunks(player_chunk);

		game_world->useAllData([&](auto& generating, auto& locations)
		{
			const auto pred = [](const auto& g) { return g->finished; };
			auto it = std::find_if(generating.begin(), generating.end(), pred);
			while (it != generating.end())
			{
				MIDNIGHT_ASSERT((*it)->chunk->model, "Error loading model");
				// take it->chunk and create entity with it->chunk.model as its mesh
				auto entity = world.entity();
				entity.set<Components::Mesh>(Components::Mesh { .model = (*it)->chunk->model });

				// then std::find(locations) for it->chunk.location and store the entity id
				const auto loc = std::find_if(locations.begin(), locations.end(), [&](const auto& loc) { return loc.first == (*it)->chunk->location; });
				MIDNIGHT_ASSERT(loc != locations.end(), "Location not found!");
				loc->second = entity.id();

                generating.erase(it);
				it = std::find_if(generating.begin(), generating.end(), pred);
			}

			// Here we can delete chunks that are far enough away
			// We want to delete locations, but only if there's an entity associated with it
			std::vector<typename std::decay_t<decltype(locations)>::iterator> to_delete;
			for (auto it = locations.begin(); it != locations.end(); it++)
			{
				if ( it->second && ( Math::length(it->first - player_chunk) > 5.f ) )
				{
					// we need to delete the entity at l.second
					to_delete.push_back(it);
					flecs::entity e(world.m_world, it->second);
					e.destruct();
				}
			}
			for (const auto& it : to_delete) locations.erase(it);

			std::cout << "There are " << generating.size() << " generating chunks whereas " << locations.size() << " have already been generated\n";
		});
	}

	void destroy() override
	{
		Game::World::destroy();
	}

	void finish() override
	{
		// request close
		_done = true;
	}

	bool done() override
	{
		return _done;
	}

private:
	bool _done;
};

int main()
{
	{
		auto engine = Game::Engine::get();
		engine->start("window.lua", []() { return new Scene(); });
	}
	Game::Engine::destroy();
}

/*
void main2()
{
    using namespace mn;
    using namespace mn::Graphics;

    auto window = Window::fromLuaScript("window.lua");

    auto pipeline = PipelineBuilder::fromLua(SOURCE_DIR, "/pipelines/main.lua")
		.setPushConstantObject<Constants>()
        .build();

	TypeBuffer<FrameInfo> frame_info;
	frame_info.resize(1);

	Vector<mn::Math::Mat4<float>> models;
	models.push_back(Math::identity<4, float>());
	models.push_back(Math::identity<4, float>());

	Model cube = Model::fromLua(SOURCE_DIR "/models/cube.lua");

	const auto& world = Game::World::get();
	world->loadFromLua(SOURCE_DIR "/world.lua");

    auto player_pos = Math::Vec3f({ 0, 9, 0 });
	auto player_rot = Math::Vec3<Math::Angle>({ Math::Angle::radians(0), Math::Angle::radians(0), Math::Angle::radians(0) });

    const auto aspect = static_cast<float>(Math::x(window.size())) / static_cast<float>(Math::y(window.size()));
	const auto set_view = [&](const auto& frame, const auto& pipeline, uint32_t index)
	{
		Constants c;
		c.buffer = models.getAddress();
		c.frame_info = frame_info.getAddress();
		c.index  = index;
		frame.setPushConstant(pipeline, c);
	};	

	// we want to be able to stage buffer data so it's not mapped

    uint32_t frameCount = 0;
    double rotation = 0.0;

	std::set<SDL_Scancode> keys;
	bool constrained = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);

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

		{
			auto& info = frame_info[0];
			info.proj  = Math::perspective(aspect, Math::Angle::degrees(75), { 0.1, 1000.0 });
			info.view  = Math::translation(player_pos * -1.f) * Math::rotation<float>(player_rot);
			info.player_pos = player_pos;
		}

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

				set_view(frame, pipeline, 0);
				for (const auto& chunk : chunks)
					frame.draw(pipeline, chunk->model);

				set_view(frame, pipeline, 1);
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

		models[1] = Math::scale(Math::Vec3f{ 0.25f, 0.25f, 0.25f }) * Math::translation(cube_pos);

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
*/
