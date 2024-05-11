#pragma once

#include <thread>
#include <midnight.hpp>

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

    struct World : mn::Utility::Singleton<World>
    {
        std::atomic<bool> done;

        struct 
        {
            int32_t chunk_cells;
            float cell_width;
        } params;

        void loadFromLua(const std::string& dir);
        void finish();
        void checkChunks(const mn::Math::Vec3i& player_chunk);
        void useChunks(const std::function<void(const std::vector<std::shared_ptr<Chunk>>&)>& func);
        float chunkSize() const { return params.chunk_cells * params.cell_width; }
        
        static bool inside(mn::Math::Vec3f position);
        static float getValue(mn::Math::Vec3f position);
        static mn::Math::Vec3f getGradient(mn::Math::Vec3f position);

    private:
        friend class Singleton;

        void generate_mesh(std::shared_ptr<Chunk> chunk);

        std::mutex chunk_mutex, generate_mutex;
        std::vector<std::shared_ptr<Chunk>> chunks;
        std::vector<std::shared_ptr<Chunk>> generate_chunks;
        std::vector<std::unique_ptr<std::thread>> worker_threads;
        std::vector<mn::Math::Vec3i> radius;

        World() : done(false)
        {	}
    };
}
