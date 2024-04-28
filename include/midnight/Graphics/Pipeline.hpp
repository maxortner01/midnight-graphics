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
        
        const auto& getDescriptorSetInfo() const { MIDNIGHT_ASSERT(type == ShaderType::Vertex, "Attributes only for vertex shader"); return descriptor_sets; }
        const auto& getAttributes() const { MIDNIGHT_ASSERT(type == ShaderType::Vertex, "Attributes only for vertex shader"); return *attributes; }

    private:
        ShaderType type;
        std::optional<std::vector<Attribute>> attributes;
        std::vector<std::shared_ptr<void>> descriptor_sets;
    };

    /*
    struct PipelineLayout : ObjectHandle<PipelineLayout>
    {
        PipelineLayout();
        ~PipelineLayout();

        auto getDescriptorSetLayout() const { return descriptor_set_layout; }

    private:
        mn::handle_t descriptor_set_layout;
    };*/

    struct PipelineBuilder;

    struct DescriptorSet
    {
        DescriptorSet(const std::vector<mn::handle_t>& layouts, uint32_t count = 1, uint32_t layout_size = 0);
        ~DescriptorSet();

        void bind(uint32_t index, const std::unique_ptr<Backend::CommandBuffer>& cmd, mn::handle_t pipelineLayout) const;

        template<typename T>
        T& data(uint32_t binding, uint32_t index)
        {
            const auto& layout = desc_layouts[binding];
            const auto& _sets = sets.at(layout);
            const auto& is = _sets[index];
            MIDNIGHT_ASSERT(sizeof(T) == is->buffer->allocated(), "Uniform type not the same as allocated! sizeof(T) = " << sizeof(T) << " and buffer->getSize() = " << is->buffer->getSize());
            return *reinterpret_cast<T*>(is->buffer->rawData());
        }

    private:
        struct IndividualSet
        {
            mn::handle_t handle;
            std::unique_ptr<Buffer> buffer;
        };

        mn::handle_t pool;
        std::vector<mn::handle_t> desc_layouts;
        std::unordered_map<mn::handle_t, std::vector<std::unique_ptr<IndividualSet>>> sets; // layout, list of sets
    };

    struct Pipeline : ObjectHandle<Pipeline>
    {
        friend struct PipelineBuilder;
        
        Pipeline(const Pipeline&) = delete;
        Pipeline(Pipeline&&) = default;

        template<typename T>
        T& descriptorData(uint32_t binding, uint32_t index)
        {
            return descriptor_set->data<T>(binding, index);
        }

        ~Pipeline();

        auto getBindingStride() const { return binding_strides[0]; }
        void bindDescriptorSet(uint32_t index, const std::unique_ptr<Backend::CommandBuffer>& cmd) const;

    private:
        Pipeline(Handle<Pipeline> h) : ObjectHandle(h) {  }

        std::vector<uint32_t> binding_strides;
        std::unique_ptr<DescriptorSet> descriptor_set;
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
        PipelineBuilder& setSize(uint32_t w, uint32_t h);
        PipelineBuilder& setColorFormat(uint32_t c);
        PipelineBuilder& setDepthFormat(uint32_t d);
        PipelineBuilder& setDescriptorCount(uint32_t c);
        PipelineBuilder& setDescriptorSize(uint32_t c);
        Pipeline build() const;

    private:
        std::pair<uint32_t, uint32_t> size;
        std::unordered_map<ShaderType, std::shared_ptr<Shader>> modules;
        Topology top  = Topology::Triangles;
        Polygon  poly = Polygon::Fill;
        bool backface_cull = true, blending = true, depth = true;
        uint32_t color_format = 0, depth_format = 0, desc_count = 1, desc_size = 0;
    };
}
