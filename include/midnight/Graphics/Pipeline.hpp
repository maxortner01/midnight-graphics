#pragma once

#include "ObjectHandle.hpp"

#include <unordered_map>
#include <vector>
#include <filesystem>

namespace mn::Graphics
{
    struct Pipeline;

    enum class ShaderType
    {
        None, Vertex, Fragment, Compute
    };

    enum class Topology
    {
        Triangles
    };

    enum class Polygon
    {
        Fill
    };

    struct Shader : ObjectHandle<Shader>
    {
        friend struct Pipeline;

        struct Attribute
        {
            uint32_t element_count, element_size, format;
            uint8_t binding = 0;
        };

        Shader();
        Shader(std::filesystem::path path, ShaderType type);
        ~Shader();

        void fromFile(std::filesystem::path path, ShaderType type);
        void fromString(const std::string& contents, ShaderType type, const std::string& path = "");
        void fromSpv(const std::vector<uint32_t>& contents, ShaderType type);

        auto getType() const { return type; }

        const auto& getAttributes() const { MIDNIGHT_ASSERT(type == ShaderType::Vertex, "Attributes only for vertex shader"); return *attributes; }

    private:
        ShaderType type;
        std::optional<std::vector<Attribute>> attributes;
    };

    struct PipelineLayout : ObjectHandle<PipelineLayout>
    {
        PipelineLayout();
        ~PipelineLayout();
    };

    struct PipelineBuilder;

    struct Pipeline : ObjectHandle<Pipeline>
    {
        friend struct PipelineBuilder;
        
        Pipeline(const Pipeline&) = delete;
        Pipeline(Pipeline&&) = default;

        ~Pipeline();

        auto getBindingStride() const { return binding_strides[0]; }

    private:
        Pipeline(Handle<Pipeline> h) : ObjectHandle(h) {  }

        std::vector<uint32_t> binding_strides;
    };

    struct PipelineBuilder
    {
        PipelineBuilder& addLayout(const std::shared_ptr<PipelineLayout>& layout);
        PipelineBuilder& createLayout();
        PipelineBuilder& addShader(std::filesystem::path path, ShaderType type);
        PipelineBuilder& addShader(const std::shared_ptr<Shader>& shader);
        PipelineBuilder& setPolyMode(Polygon p);
        PipelineBuilder& setTopology(Topology t);
        PipelineBuilder& setBackfaceCull(bool cull);
        PipelineBuilder& setBlending(bool blend);
        PipelineBuilder& setDepthTesting(bool d);
        PipelineBuilder& setSize(uint32_t w, uint32_t h);
        PipelineBuilder& setColorFormat(uint32_t c);
        PipelineBuilder& setDepthFormat(uint32_t d);
        Pipeline build() const;

    private:
        std::pair<uint32_t, uint32_t> size;
        std::shared_ptr<PipelineLayout> _layout;
        std::unordered_map<ShaderType, std::shared_ptr<Shader>> modules;
        Topology top  = Topology::Triangles;
        Polygon  poly = Polygon::Fill;
        bool backface_cull = true, blending = true, depth = true;
        uint32_t color_format = 0, depth_format = 0;
    };
}
