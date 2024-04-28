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
            mn::Math::Vec3f position;
            mn::Math::Vec4f color;
        };

        struct Frame
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
        };

        static Model fromFrame(const Frame& frame);
        static Model fromLua(const std::string& lua_file);

        std::size_t vertexCount() const;
        std::size_t indexCount() const;

        void setVertexCount(uint32_t count);
        void setIndexCount(uint32_t count);

        std::span<Vertex> vertices();
        std::span<const Vertex> vertices() const;

        std::span<uint32_t> indices();
        std::span<const uint32_t> indices() const;

    private:
        std::shared_ptr<Buffer> vertex, index;
    };
}