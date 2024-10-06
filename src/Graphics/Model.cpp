#include <Graphics/Model.hpp>
#include <Graphics/Buffer.hpp>

#include <SL/Lua.hpp>

namespace mn::Graphics
{

Model Model::fromFrame(const Frame& frame)
{
    Model m;

    if (frame.vertices.size())
    {
        m.vertex = std::make_shared<Buffer>();
        m.vertex->allocateBytes(frame.vertices.size() * sizeof(Vertex));

        std::memcpy(
            m.vertex->rawData(), 
            reinterpret_cast<const Vertex*>(&frame.vertices[0]), 
            frame.vertices.size() * sizeof(Vertex));
    }

    if (frame.indices.size())
    {
        m.index = std::make_shared<Buffer>();
        m.index->allocateBytes(frame.indices.size() * sizeof(uint32_t));
        std::memcpy(
            m.index->rawData(), 
            reinterpret_cast<const uint32_t*>(&frame.indices[0]), 
            frame.indices.size() * sizeof(uint32_t));
    }

    return m;
}

Model Model::fromLua(const std::string& lua_file)
{
    Frame f;

    SL::Runtime runtime(lua_file);

    const auto model = runtime.getGlobal<SL::Table>("Model");
    MIDNIGHT_ASSERT(model, "Error loading model: Model global not found!");

    model->try_get<SL::Table>("vertices", [&](const SL::Table& vertices)
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

    model->try_get<SL::Table>("colors", [&](const SL::Table& colors)
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

    model->try_get<SL::Table>("normals", [&](const SL::Table& normals)
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

    model->try_get<SL::Table>("indices", [&](const SL::Table& indices)
    {
        indices.each<SL::Number>([&](uint32_t i, const SL::Number& index)
        { f.indices.push_back(index); });
    });

    return fromFrame(f);
}

std::size_t Model::vertexCount() const
{
    return (vertex ? vertex->allocated() / sizeof(Vertex) : 0);
}

std::size_t Model::indexCount() const
{
    return (index ? index->allocated() / sizeof(uint32_t) : 0);
}

void Model::setVertexCount(uint32_t count)
{
    if (!vertex)
        vertex = std::make_shared<Buffer>();

    vertex->allocateBytes(sizeof(Vertex) * count);
}   

void Model::setIndexCount(uint32_t count)
{
    if (!index)
        index = std::make_shared<Buffer>();

    index->allocateBytes(sizeof(uint32_t) * count);
}

std::span<Model::Vertex> Model::vertices()
{
    using s = std::span<Vertex>;
    return (vertex ? 
    s{
        reinterpret_cast<Vertex*>(vertex->rawData()),
        vertexCount() 
    } : s());
}

std::span<const Model::Vertex> Model::vertices() const
{
    using s = std::span<const Vertex>;
    return (vertex ? 
    s{
        reinterpret_cast<const Vertex*>(vertex->rawData()),
        vertexCount() 
    } : s());
}

std::span<uint32_t> Model::indices()
{
    using s = std::span<uint32_t>;
    return (index ? 
    s{
        reinterpret_cast<uint32_t*>(index->rawData()),
        indexCount() 
    } : s());
}

std::span<const uint32_t> Model::indices() const
{
    using s = std::span<const uint32_t>;
    return (index ? 
    s{
        reinterpret_cast<const uint32_t*>(index->rawData()),
        indexCount() 
    } : s());
}

}