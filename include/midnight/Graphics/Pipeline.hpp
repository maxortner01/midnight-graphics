#pragma once

#include "Buffer.hpp"
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
        Fill, Wireframe
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

    struct PipelineBuilder;

    struct Pipeline : ObjectHandle<Pipeline>
    {
        friend struct PipelineBuilder;
        
        Pipeline(const Pipeline&) = delete;
        Pipeline(Pipeline&&) = default;

        ~Pipeline();

        auto getBindingStride() const { return binding_strides[0]; }

        void setPushConstant(const std::unique_ptr<Backend::CommandBuffer>& cmd, const void* data) const;
        template<typename T>
        void setPushConstant(const std::unique_ptr<Backend::CommandBuffer>& cmd, const T& value) const
        {
            MIDNIGHT_ASSERT(sizeof(T) == push_constant_size, "Push constant size descrepancy");
            setPushConstant(cmd, reinterpret_cast<const void*>(&value));
        }

    private:

        Pipeline(Handle<Pipeline> h) : ObjectHandle(h) {  }

        uint32_t push_constant_size;
        std::vector<uint32_t> binding_strides;
        mn::handle_t layout;
    };

    struct PipelineBuilder
    {

        static PipelineBuilder fromLua(const std::string& source_dir, const std::string& script);

        PipelineBuilder& addShader(std::filesystem::path path, ShaderType type);
        PipelineBuilder& addShader(const std::shared_ptr<Shader>& shader);
        PipelineBuilder& setPolyMode(Polygon p);
        PipelineBuilder& setTopology(Topology t);
        PipelineBuilder& setBackfaceCull(bool cull);
        PipelineBuilder& setBlending(bool blend);
        PipelineBuilder& setDepthTesting(bool d);
        PipelineBuilder& setCullDirection(bool clockwise);
        PipelineBuilder& setSize(uint32_t w, uint32_t h);
        PipelineBuilder& setColorFormat(uint32_t c);
        PipelineBuilder& setDepthFormat(uint32_t d);
        
        template<typename T>
        PipelineBuilder& setPushConstantObject()
        {
            push_constant_size = sizeof(T);
            return *this;
        }

        Pipeline build() const;

    private:
        std::pair<uint32_t, uint32_t> size;
        std::unordered_map<ShaderType, std::shared_ptr<Shader>> modules;
        Topology top  = Topology::Triangles;
        Polygon  poly = Polygon::Fill;
        bool backface_cull = true, blending = true, depth = true, clockwise = true;
        uint32_t color_format = 0, depth_format = 0, push_constant_size = 0;
    };
}
