#pragma once

#include <Def.hpp>
#include <Math.hpp>

namespace mn::Graphics
{
    template<typename T>
    struct TypeBuffer;
    struct RenderFrame;

    struct Mesh
    {
        friend struct RenderFrame;

        struct Vertex
        {
            mn::Math::Vec3f position, normal;
            mn::Math::Vec4f color;
            mn::Math::Vec2f tex_coords;
        };

        struct Frame
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
        };

        MN_SYMBOL static Mesh fromFrame(const Frame& frame);
        MN_SYMBOL static Mesh fromLua(const std::string& lua_file);

        MN_SYMBOL std::size_t vertexCount() const;
        MN_SYMBOL std::size_t indexCount() const;

        MN_SYMBOL void setVertexCount(uint32_t count);
        MN_SYMBOL void setIndexCount(uint32_t count);

        MN_SYMBOL std::span<Vertex> vertices();
        MN_SYMBOL std::span<const Vertex> vertices() const;

        MN_SYMBOL std::span<uint32_t> indices();
        MN_SYMBOL std::span<const uint32_t> indices() const;

        MN_SYMBOL std::size_t allocated() const;

        std::shared_ptr<TypeBuffer<Vertex>> vertex;
        std::shared_ptr<TypeBuffer<uint32_t>> index;
    };
}