#include "World.hpp"

#include <SimplexNoise.h>
#include <SL/Lua.hpp>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

#define START_TIMER(name) const auto name = std::chrono::high_resolution_clock::now()
#define MEASURE_SECONDS(name) [name]() { std::chrono::duration<double, std::chrono::seconds::period> dt = std::chrono::high_resolution_clock::now() - name; return dt.count(); }()

namespace Game
{

static inline std::vector<mn::Math::Vec2u> edge_vertex_indices;
static inline std::vector<std::vector<int32_t>> triangle_table;

SimplexNoise noise;
std::mutex m;

void load_edge_vertex_indices()
{
    SL::Runtime runtime(SOURCE_DIR "/mesh/MCTables.lua");
    assert(runtime);

    const auto vertex_indices = runtime.getGlobal<SL::Table>("EdgeVertexIndices");
    assert(vertex_indices);

    vertex_indices->each<SL::Table>([](uint32_t i, const SL::Table& edge)
    {
        edge_vertex_indices.push_back(mn::Math::Vec2u({ (uint32_t)edge.get<SL::Number>("1"), (uint32_t)edge.get<SL::Number>("2") }));
    });
    edge_vertex_indices.shrink_to_fit();
}

void load_triangle_table()
{
    SL::Runtime runtime(SOURCE_DIR "/mesh/MCTables.lua");
    assert(runtime);

    const auto triangles = runtime.getGlobal<SL::Table>("TriangleTable");
    assert(triangles);

    triangles->each<SL::Table>([](uint32_t i, const SL::Table& t)
    {
        std::vector<int32_t> ts;
        t.each<SL::Number>([&ts](uint32_t, const SL::Number& index)
        {
            ts.push_back(index);
        });
        triangle_table.push_back(ts);
    });
    triangle_table.shrink_to_fit();
}

void World::loadFromLua(const std::string& dir)
{
    SL::Runtime runtime(dir);
    MIDNIGHT_ASSERT(runtime, "Error loading world from lua script");

    const auto world = runtime.getGlobal<SL::Table>("World");
    uint32_t workers = world->get<SL::Number>("Workers");
    params.cell_width = world->get<SL::Number>("CellWidth");
    params.chunk_cells = world->get<SL::Number>("ChunkCells");

    const auto& r = world->get<SL::Table>("Radius");
    r.each<SL::Table>([this](uint32_t, const SL::Table& pos)
    {
        this->radius.push_back({
            static_cast<int32_t>(pos.get<SL::Number>("1")), 
            static_cast<int32_t>(pos.get<SL::Number>("2")), 
            static_cast<int32_t>(pos.get<SL::Number>("3"))
        });
    });

    const auto thread_func = [&]()
    {
        while (!done)
        {
            if (!generate_chunks.size()) continue;

            generate_mutex.lock();
            auto chunk = generate_chunks.back();
            generate_chunks.pop_back();
            generate_mutex.unlock();

            chunk->model = this->generate_mesh(chunk->location);

            chunk_mutex.lock();
            chunks.push_back(chunk);
            chunk_mutex.unlock();
        }
    };

    for (uint32_t i = 0; i < workers; i++)
        worker_threads.push_back(std::make_unique<std::thread>(thread_func));
}

void World::finish()
{
    done = true;
    for (const auto& g : worker_threads)
        g->join();
}

void World::checkChunks(const mn::Math::Vec3i& player_chunk)
{
    using namespace mn;

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

    // prioritize the chunks closest tot he player
    std::sort(generate_chunks.begin(), generate_chunks.end(), [player_chunk](const auto& a, const auto& b) { return Math::length(b->location - player_chunk) < Math::length(a->location - player_chunk); });
    // cull the chunks that are far enough away
    std::erase_if(generate_chunks, [player_chunk](const auto& a) { return Math::length(a->location - player_chunk) >= 4; });

    chunk_mutex.lock();
    // cull the ones far enough away
    std::erase_if(chunks, [player_chunk](const auto& a) { return Math::length(a->location - player_chunk) >= 3; });
    chunk_mutex.unlock();

    generate_mutex.unlock();
}

void World::useChunks(const std::function<void(const std::vector<std::shared_ptr<Chunk>>&)>& func)
{
    std::lock_guard lock(chunk_mutex);
    func(chunks);
}

std::shared_ptr<mn::Graphics::Model> World::generate_mesh(mn::Math::Vec3i chunk_index)
{
    START_TIMER(main);

    using namespace mn;
    using namespace mn::Graphics;

    {
        std::lock_guard lock(m);
        if (!edge_vertex_indices.size())
            load_edge_vertex_indices();
        if (!triangle_table.size())
            load_triangle_table();
    }

    Model::Frame frame;
    
    // depth = height = width = CHUNK_CELLS * 2 + 1
    const auto CHUNK_SIZE = params.cell_width * params.chunk_cells;
    const auto SIZE = params.chunk_cells * 2 + 1;

    const auto center = Math::Vec3f({ 
		Math::x(chunk_index) * CHUNK_SIZE, 
		Math::y(chunk_index) * CHUNK_SIZE,
		Math::z(chunk_index) * CHUNK_SIZE });

    const auto world_pos = [&](Math::Vec3u index)
    {
        return Math::Vec3f({
            ((int32_t)Math::x(index) - params.chunk_cells) * params.cell_width + Math::x(center),
            ((int32_t)Math::y(index) - params.chunk_cells) * params.cell_width + Math::y(center),
            ((int32_t)Math::z(index) - params.chunk_cells) * params.cell_width + Math::z(center)
        });
    };

    const auto value = [](Math::Vec3f pos)
    {
        return noise.fractal(5, Math::x(pos) * 0.025, Math::y(pos) * 0.025, Math::z(pos) * 0.025);
    };

	struct Pos
	{
		Math::Vec3f world_position;
		float value;
	};

	std::vector<std::vector<std::vector<Pos>>> values;
	values.resize(SIZE);
	for (auto& plane : values)
	{
		plane.resize(SIZE);
		for (auto& line : plane)
			line.resize(SIZE);
	}

	for (uint32_t z = 0; z < SIZE; z++)
		for (uint32_t y = 0; y < SIZE; y++)
			for (uint32_t x = 0; x < SIZE; x++)
			{
				const auto world_position = world_pos({x, y, z});
				const auto v = value(world_position);
				values[z][y][x] = Pos{
					.value = v,
					.world_position = world_position
				};
			}

    const auto cutoff = 0.04f;

    START_TIMER(vertex_timer);

	frame.vertices.reserve(SIZE * SIZE * SIZE * 5);
	frame.indices.reserve(SIZE * SIZE * SIZE * 5);
	Pos* positions[8] = {  };

    for (uint32_t z = 0; z < SIZE - 1; z++)
        for (uint32_t y = 0; y < SIZE - 1; y++)
            for (uint32_t x = 0; x < SIZE - 1; x++)
            {	
				positions[0] = &values[z + 1][y    ][x    ];
				positions[1] = &values[z + 1][y    ][x + 1];
				positions[2] = &values[z + 1][y + 1][x    ];
				positions[3] = &values[z + 1][y + 1][x + 1];

				positions[4] = &values[z    ][y    ][x    ];
				positions[5] = &values[z    ][y    ][x + 1];
				positions[6] = &values[z    ][y + 1][x    ];
				positions[7] = &values[z    ][y + 1][x + 1];

                uint8_t index = 0;
                for (uint32_t i = 0; i < 8; i++)
                    if (positions[i]->value > cutoff)
                        index |= (1 << i);
                
                assert(index < triangle_table.size());
                const auto& triangle = triangle_table[index];
                for (const auto& tri_index : triangle)
                {
                    if (tri_index == -1) break;
                    
                    assert(tri_index < edge_vertex_indices.size());
                    const auto& edge = edge_vertex_indices[tri_index];

					Math::Vec3f position = 
						positions[Math::x(edge)]->world_position + 
							(positions[Math::y(edge)]->world_position - positions[Math::x(edge)]->world_position) * 
							( ( cutoff - positions[Math::x(edge)]->value ) / ( positions[Math::y(edge)]->value - positions[Math::x(edge)]->value ) );

					frame.vertices.push_back(Model::Vertex {
						.position = position
					});
                }
            }

	//frame.indices.shrink_to_fit();
	//frame.vertices.shrink_to_fit();
    std::cout << "Vertices generated in " << MEASURE_SECONDS(vertex_timer) << " seconds\n";

    START_TIMER(index_timer);

    for (uint32_t i = 0; i < frame.vertices.size(); i += 3)
    {
		const uint32_t indices[] = { i, i + 1, i + 2 };
		const auto v1 = frame.vertices[indices[1]].position - frame.vertices[indices[0]].position;
		const auto v2 = frame.vertices[indices[2]].position - frame.vertices[indices[0]].position;
		const auto normal = Math::normalized(Math::outer(v1, v2));
		for (uint32_t j = 0; j < 3; j++)
		{
			frame.vertices[indices[j]].normal = normal;
			frame.indices.push_back(frame.indices.size());
		}
    } 

    std::cout << "Normals generated in " << MEASURE_SECONDS(index_timer) << " seconds\n";

    std::cout << "Chunk generated in " << MEASURE_SECONDS(main) << " seconds..." << std::endl;
	std::cout << "  Vertex count: " << frame.vertices.size() << " (" << frame.vertices.size() * sizeof(Model::Vertex) * 1e-6 << ") MB\n";
	std::cout << "  Index count:  " << frame.indices.size() << " (" << frame.indices.size() * sizeof(uint32_t) * 1e-6 << ") MB\n\n";

    return std::make_shared<Model>(Model::fromFrame(frame));
}


}