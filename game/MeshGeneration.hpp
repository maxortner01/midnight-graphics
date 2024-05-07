#pragma once

#include <midnight.hpp>

namespace Game
{
    inline static const auto CHUNK_CELLS = 100 / 2;
    inline static const auto CELL_WIDTH = 0.2f * 2.f;
    inline static const auto CHUNK_SIZE = CHUNK_CELLS * CELL_WIDTH;

    std::shared_ptr<mn::Graphics::Model> generate_mesh(mn::Math::Vec3i chunk_index);
}