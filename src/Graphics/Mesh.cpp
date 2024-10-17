#include <Graphics/Mesh.hpp>
#include <Graphics/Buffer.hpp>

#include <SL/Lua.hpp>

namespace mn::Graphics
{

Mesh Mesh::fromFrame(const Frame& frame)
{
    Mesh m;

    if (frame.vertices.size())
    {
        m.vertex = std::make_shared<TypeBuffer<Vertex>>();
        m.vertex->resize(frame.vertices.size());

        std::memcpy(
            &m.vertex->at(0), 
            reinterpret_cast<const Vertex*>(&frame.vertices[0]), 
            frame.vertices.size() * sizeof(Vertex));
    }

    if (frame.indices.size())
    {
        m.index = std::make_shared<TypeBuffer<uint32_t>>();
        m.index->resize(frame.indices.size());
        std::memcpy(
            &m.index->at(0), 
            reinterpret_cast<const uint32_t*>(&frame.indices[0]), 
            frame.indices.size() * sizeof(uint32_t));
    }

    return m;
}

Mesh Mesh::fromLua(const std::string& lua_file)
{
    Frame f;

    SL::Runtime runtime(lua_file);

    const auto Mesh = runtime.getGlobal<SL::Table>("Mesh");
    MIDNIGHT_ASSERT(Mesh, "Error loading Mesh: Mesh global not found!");

    Mesh->try_get<SL::Table>("vertices", [&](const SL::Table& vertices)
    {
        vertices.each<SL::Table>([&](uint32_t i, const SL::Table& vertex)
        {
            Vertex v;
            v.position = { 
                vertex.get<SL::Number>("1"),
                vertex.get<SL::Number>("2"),
                vertex.get<SL::Number>("3") 
            };
            f.vertices.push_back(v);
        });
    });

    Mesh->try_get<SL::Table>("colors", [&](const SL::Table& colors)
    {
        colors.each<SL::Table>([&](uint32_t i, const SL::Table& color)
        {
            f.vertices[i - 1].color = { 
                color.get<SL::Number>("1"),
                color.get<SL::Number>("2"),
                color.get<SL::Number>("3"),
                color.get<SL::Number>("4") 
            };
        });
    });

    Mesh->try_get<SL::Table>("normals", [&](const SL::Table& normals)
    {
        normals.each<SL::Table>([&](uint32_t i, const SL::Table& normal)
        {
            f.vertices[i - 1].normal = { 
                normal.get<SL::Number>("1"),
                normal.get<SL::Number>("2"),
                normal.get<SL::Number>("3"),
                normal.get<SL::Number>("4") 
            };
        });
    });

    Mesh->try_get<SL::Table>("indices", [&](const SL::Table& indices)
    {
        indices.each<SL::Number>([&](uint32_t i, const SL::Number& index)
        { f.indices.push_back(index); });
    });

    return fromFrame(f);
}

std::size_t Mesh::vertexCount() const
{
    return (vertex ? vertex->size() : 0);
}

std::size_t Mesh::indexCount() const
{
    return (index ? index->size() : 0);
}

void Mesh::setVertexCount(uint32_t count)
{
    if (!vertex)
        vertex = std::make_shared<TypeBuffer<Vertex>>();

    vertex->resize(count);
}   

void Mesh::setIndexCount(uint32_t count)
{
    if (!index)
        index = std::make_shared<TypeBuffer<uint32_t>>();

    index->resize(count);
}

std::span<Mesh::Vertex> Mesh::vertices()
{
    using s = std::span<Vertex>;
    return (vertex ? 
    s{
        reinterpret_cast<Vertex*>(vertex->rawData()),
        vertexCount() 
    } : s());
}

std::span<const Mesh::Vertex> Mesh::vertices() const
{
    using s = std::span<const Vertex>;
    return (vertex ? 
    s{
        reinterpret_cast<const Vertex*>(vertex->rawData()),
        vertexCount() 
    } : s());
}

std::span<uint32_t> Mesh::indices()
{
    using s = std::span<uint32_t>;
    return (index ? 
    s{
        reinterpret_cast<uint32_t*>(index->rawData()),
        indexCount() 
    } : s());
}

std::span<const uint32_t> Mesh::indices() const
{
    using s = std::span<const uint32_t>;
    return (index ? 
    s{
        reinterpret_cast<const uint32_t*>(index->rawData()),
        indexCount() 
    } : s());
}

std::size_t Mesh::allocated() const
{
    return ( vertex ? vertex->allocated() : 0U ) + ( index ? index->allocated() : 0U );
}

}