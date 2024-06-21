#pragma once

#include <Def.hpp>
#include <Math.hpp>

namespace mn::Graphics
{
    struct Buffer;
    struct RenderFrame;

    struct Model
    {
        friend struct RenderFrame;

        struct Vertex
        {
            mn::Math::Vec3f position, normal;
            mn::Math::Vec4f color;
        };

        struct Frame
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
        };

        MN_SYMBOL static Model fromFrame(const Frame& frame);
        MN_SYMBOL static Model fromLua(const std::string& lua_file);

        MN_SYMBOL std::size_t vertexCount() const;
        MN_SYMBOL std::size_t indexCount() const;

        MN_SYMBOL void setVertexCount(uint32_t count);
        MN_SYMBOL void setIndexCount(uint32_t count);

        MN_SYMBOL std::span<Vertex> vertices();
        MN_SYMBOL std::span<const Vertex> vertices() const;

        MN_SYMBOL std::span<uint32_t> indices();
        MN_SYMBOL std::span<const uint32_t> indices() const;

    private:
        std::shared_ptr<Buffer> vertex, index;
    };
}