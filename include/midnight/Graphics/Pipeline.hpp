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
            u32 element_count, element_size, format;
            u8 binding = 0;
        };

        MN_SYMBOL Shader();
        MN_SYMBOL Shader(std::filesystem::path path, ShaderType type);
        MN_SYMBOL ~Shader();

        MN_SYMBOL void fromFile(std::filesystem::path path, ShaderType type);
        MN_SYMBOL void fromString(const std::string& contents, ShaderType type, const std::string& path = "");
        MN_SYMBOL void fromSpv(const std::vector<uint32_t>& contents, ShaderType type);

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

        MN_SYMBOL ~Pipeline();

        auto getBindingStride() const { return binding_strides[0]; }

        MN_SYMBOL void setPushConstant(const std::unique_ptr<Backend::CommandBuffer>& cmd, const void* data) const;

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

        MN_SYMBOL static PipelineBuilder fromLua(const std::string& source_dir, const std::string& script);

        MN_SYMBOL PipelineBuilder& addShader(std::filesystem::path path, ShaderType type);
        MN_SYMBOL PipelineBuilder& addShader(const std::shared_ptr<Shader>& shader);
        MN_SYMBOL PipelineBuilder& setPolyMode(Polygon p);
        MN_SYMBOL PipelineBuilder& setTopology(Topology t);
        MN_SYMBOL PipelineBuilder& setBackfaceCull(bool cull);
        MN_SYMBOL PipelineBuilder& setBlending(bool blend);
        MN_SYMBOL PipelineBuilder& setDepthTesting(bool d);
        MN_SYMBOL PipelineBuilder& setCullDirection(bool clockwise);
        MN_SYMBOL PipelineBuilder& setSize(uint32_t w, uint32_t h);
        MN_SYMBOL PipelineBuilder& setColorFormat(uint32_t c);
        MN_SYMBOL PipelineBuilder& setDepthFormat(uint32_t d);
        
        template<typename T>
        PipelineBuilder& setPushConstantObject()
        {
            push_constant_size = sizeof(T);
            return *this;
        }

        MN_SYMBOL Pipeline build() const;

    private:
        std::pair<uint32_t, uint32_t> size;
        std::unordered_map<ShaderType, std::shared_ptr<Shader>> modules;
        Topology top  = Topology::Triangles;
        Polygon  poly = Polygon::Fill;
        bool backface_cull = true, blending = true, depth = true, clockwise = true;
        uint32_t color_format = 0, depth_format = 0, push_constant_size = 0;
    };
}
