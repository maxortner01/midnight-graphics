#pragma once

#include <flecs.h>
#include <thread>
#include <midnight.hpp>
#include <atomic>

namespace Game
{
    struct Chunk
    {
        Chunk(const mn::Math::Vec3i& chunk_index) :
            location(chunk_index)
        {   }

        const mn::Math::Vec3i location;

        std::shared_ptr<mn::Graphics::Model> model;
    };

    struct ChunkComp
    {
        mn::Math::Vec3i location;
    };

    struct World : mn::Utility::Singleton<World>
    {
        std::atomic<bool> done;

        struct Generating
        {
            std::shared_ptr<Chunk> chunk;
            bool chosen = false, finished = false;
        };

        struct 
        {
            int32_t chunk_cells;
            float cell_width;
        } params;

        void loadFromLua(const std::string& dir);
        void finish();
        void checkChunks(const mn::Math::Vec3i& player_chunk);
        float chunkSize() const { return params.chunk_cells * params.cell_width; }

        static bool inside(mn::Math::Vec3f position);
        static float getValue(mn::Math::Vec3f position);
        static mn::Math::Vec3f getGradient(mn::Math::Vec3f position);

        ~World() { finish(); };

    private:
        friend class Singleton;

        std::shared_ptr<mn::Graphics::Model> generate_mesh(std::shared_ptr<Chunk> chunk);

        std::mutex location_mutex, generate_mutex;
        std::vector<std::pair<mn::Math::Vec3i, uint64_t>> locations;
        std::vector<std::shared_ptr<Generating>> generating;
        
        std::vector<std::unique_ptr<std::thread>> worker_threads;
        std::vector<mn::Math::Vec3i> radius;

        World() : done(false)
        {	}
    
    public:

        void useAllData(std::function<void(std::vector<std::shared_ptr<Generating>>&, std::vector<std::pair<mn::Math::Vec3i, uint64_t>>&)> func)
        {
            location_mutex.lock();
            generate_mutex.lock();
            func(generating, locations);
            generate_mutex.unlock();
            location_mutex.unlock();
        }
    };
}
