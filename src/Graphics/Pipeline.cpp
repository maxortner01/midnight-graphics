#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Pipeline.hpp>
#include <Graphics/Buffer.hpp>
#include <Graphics/Backend/Command.hpp>

#include <Def.hpp>

#include <iostream>
#include <cassert>
#include <exception>
#include <sstream>
#include <fstream>

#include <shaderc/shaderc.hpp>

#include <SL/Lua.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <spirv_reflect.h>

namespace mn::Graphics
{

Shader::Shader() : type(ShaderType::None)
{   }

Shader::Shader(std::filesystem::path path, ShaderType type)
{
    fromFile(path, type);
}

Shader::~Shader()
{
    if (handle)
    {
        auto instance = Backend::Instance::get();
        instance->getDevice()->destroyShader(handle);
        handle = nullptr;
    }
}

void Shader::fromFile(std::filesystem::path path, ShaderType type)
{
    std::ifstream file(path, std::ios::in);
    assert(file);

    std::stringstream ss;
    ss << file.rdbuf();

    return fromString(ss.str(), type, path.filename().string());
}

void Shader::fromString(const std::string& contents, ShaderType type, const std::string& path)
{
    using namespace shaderc;

    shaderc_shader_kind kind;
    switch (type)
    {
    case ShaderType::Vertex:   kind = shaderc_vertex_shader;   break;
    case ShaderType::Fragment: kind = shaderc_fragment_shader; break;
    default: MIDNIGHT_ASSERT(false, "Only vertex and fragment shaders currently supported");
    }

    Compiler compiler;
    CompileOptions options;
    const auto result = compiler.CompileGlslToSpv(contents, kind, path.c_str());
    MIDNIGHT_ASSERT(result.GetCompilationStatus() == shaderc_compilation_status_success, "Error compiling shader: '" << path << "'\n" << result.GetErrorMessage());
    MIDNIGHT_ASSERT(result.cbegin(), "Error compiling shader '" << path << "'");

    std::cout << "Shader '" << path << "' compiled successfully\n";

    std::vector<uint32_t> data;
    data.reserve(result.cend() - result.cbegin());
    for (const auto* it = result.cbegin(); it != result.cend(); it++)
        data.push_back(*it);

    return fromSpv(data, type);
}

struct DescriptorSetData
{
    uint32_t set_number;
    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

void Shader::fromSpv(const std::vector<uint32_t>& data, ShaderType type)
{
    auto instance = Backend::Instance::get();
    handle = instance->getDevice()->createShader(data);
    MIDNIGHT_ASSERT(handle, "Shader creation failed");

    this->type = type;

    if (type == ShaderType::Vertex)
    {
        // Shader reflection
        SpvReflectShaderModule shader_mod;
        const auto result = spvReflectCreateShaderModule(data.size() * sizeof(uint32_t), data.data(), &shader_mod);
        MIDNIGHT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Error during shader reflection");
        
        uint32_t input_count;
        spvReflectEnumerateInputVariables(&shader_mod, &input_count, nullptr);
        std::vector<SpvReflectInterfaceVariable*> vars(input_count);
        spvReflectEnumerateInputVariables(&shader_mod, &input_count, vars.data());

        attributes.emplace(std::vector<Attribute>());
        for (const auto* var : vars)
        {
            uint32_t member_count = 0;
            if (var->format >= 98 && var->format <= 100)
                member_count = 1;
            if (var->format >= 101 && var->format <= 103)
                member_count = 2;
            if (var->format >= 104 && var->format <= 106)
                member_count = 3;
            if (var->format >= 107 && var->format <= 109)
                member_count = 4;

            attributes->push_back(Attribute {
                .element_count = member_count,
                .format = static_cast<uint32_t>(var->format),
                .binding = 0,
                .element_size = sizeof(float)
            });
        }

        // Descriptor sets
        uint32_t desc_count;
        spvReflectEnumerateDescriptorSets(&shader_mod, &desc_count, nullptr);
        std::vector<SpvReflectDescriptorSet*> sets(desc_count);
        spvReflectEnumerateDescriptorSets(&shader_mod, &desc_count, sets.data());

        for (const auto* set : sets)
        {
            auto desc_data = std::make_shared<DescriptorSetData>();
            desc_data->bindings.resize(set->binding_count);

            for (uint32_t i = 0; i < set->binding_count; i++)
            {
                const auto* binding = *(set->bindings + i);
                desc_data->bindings.at(i).stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                desc_data->bindings.at(i).binding = binding->binding;
                desc_data->bindings.at(i).descriptorCount = 1;
                desc_data->bindings.at(i).descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type);

                for (uint32_t j = 0; j < binding->array.dims_count; j++)
                    desc_data->bindings.at(i).descriptorCount *= binding->array.dims[j];
            }

            desc_data->set_number = set->set;
            desc_data->create_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .bindingCount = set->binding_count,
                .pBindings = desc_data->bindings.data()
            };

            descriptor_sets.push_back(std::reinterpret_pointer_cast<void>(desc_data));
        }

        spvReflectDestroyShaderModule(&shader_mod);
    }
}

/*
PipelineLayout::PipelineLayout()
{   
    auto& device = Backend::Instance::get()->getDevice();

    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo desc_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = 0,
        .pNext = nullptr,
        .bindingCount = 1,
        .pBindings = &binding
    };
    
    VkDescriptorSetLayout desc_layout;
    auto err = vkCreateDescriptorSetLayout(device->getHandle().as<VkDevice>(), &desc_create_info, nullptr, &desc_layout);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating descriptor set layout");
    descriptor_set_layout = desc_layout;

    VkPipelineLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
        .setLayoutCount = 1,
        .pSetLayouts = &desc_layout
    };

    VkPipelineLayout layout;
    err = vkCreatePipelineLayout(device->getHandle().as<VkDevice>(), &create_info, nullptr, &layout);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating pipeline layout");
    handle = layout;
}

PipelineLayout::~PipelineLayout()
{
    if (handle)
    {
        auto& device = Backend::Instance::get()->getDevice();
        vkDestroyPipelineLayout(device->getHandle().as<VkDevice>(), handle.as<VkPipelineLayout>(), nullptr);
        handle = nullptr;
    }

    if (descriptor_set_layout)
    {
        auto& device = Backend::Instance::get()->getDevice();
        vkDestroyDescriptorSetLayout(device->getHandle().as<VkDevice>(), static_cast<VkDescriptorSetLayout>(descriptor_set_layout), nullptr);
        descriptor_set_layout = nullptr;
    }
}*/

DescriptorSet::DescriptorSet(const std::vector<mn::handle_t>& layouts, uint32_t count, uint32_t layout_size) :
    desc_layouts(layouts)
{
    auto& device = Backend::Instance::get()->getDevice();

    VkDescriptorPoolSize pool_size = {
        .descriptorCount = count,
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    };

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
        .maxSets = count
    };

    VkDescriptorPool _pool;
    auto err = vkCreateDescriptorPool(
        device->getHandle().as<VkDevice>(),
        &pool_create_info, 
        nullptr,
        &_pool);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating descriptor pool: " << err);
    pool = _pool;

    for (const auto& layout : layouts)
    {
        std::vector<VkDescriptorSetLayout> LAYOUTS(count, static_cast<VkDescriptorSetLayout>(layout));

        VkDescriptorSetAllocateInfo info = {
           .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
           .pNext = nullptr,
           .descriptorPool = _pool,
           .descriptorSetCount = count,
           .pSetLayouts = LAYOUTS.data()
        };

        std::vector<mn::handle_t> _sets(count);
        const auto err = vkAllocateDescriptorSets(device->getHandle().as<VkDevice>(), &info, reinterpret_cast<VkDescriptorSet*>(_sets.data()));
        MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error allocating descriptor sets: " << err);

        sets.emplace(layout, [&]()
        {
            std::vector<std::unique_ptr<IndividualSet>> individual_sets;
            for (const auto& set : _sets)
            {
                individual_sets.emplace_back([&]()
                {
                    auto is = std::make_unique<IndividualSet>();
                    is->handle = set;
                    is->buffer = std::make_unique<Buffer>();
                    
                    is->buffer->allocateBytes(layout_size);
                    
                    VkDescriptorBufferInfo buffer_info = {
                        .range = layout_size,
                        .offset = 0,
                        .buffer = is->buffer->getHandle().as<VkBuffer>()
                    };

                    VkWriteDescriptorSet write_set = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstBinding = static_cast<uint32_t>(sets.size()),
                        .dstSet = static_cast<VkDescriptorSet>(set),
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .pBufferInfo = &buffer_info
                    };

                    vkUpdateDescriptorSets(device->getHandle().as<VkDevice>(), 1, &write_set, 0, nullptr);

                    return is;
                }());
            }
            return individual_sets;
        }());
    }
}

DescriptorSet::~DescriptorSet()
{
    auto& device = Backend::Instance::get()->getDevice();

    if (pool)
    {
        vkDestroyDescriptorPool(device->getHandle().as<VkDevice>(), static_cast<VkDescriptorPool>(pool), nullptr);
        pool = nullptr;
    }

    if (desc_layouts.size())
    {
        for (const auto& desc_layout : desc_layouts)
            vkDestroyDescriptorSetLayout(device->getHandle().as<VkDevice>(), static_cast<VkDescriptorSetLayout>(desc_layout), nullptr);
        desc_layouts.clear();
    }
}

void DescriptorSet::bind(uint32_t index, const std::unique_ptr<Backend::CommandBuffer>& cmd, mn::handle_t pipelineLayout) const
{
    std::vector<VkDescriptorSet> _sets;
    _sets.reserve(sets.size());
    for (const auto& p : sets)
        _sets.push_back(static_cast<VkDescriptorSet>(p.second[index]->handle));
    vkCmdBindDescriptorSets(
        cmd->getHandle().as<VkCommandBuffer>(),
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        static_cast<VkPipelineLayout>(pipelineLayout),
        0, _sets.size(),
        _sets.data(),
        0, nullptr
    );
}

Pipeline::~Pipeline()
{
    auto& device = Backend::Instance::get()->getDevice();

    handle.destroy<VkPipeline>([&device](VkPipeline pipeline)
    {
        vkDestroyPipeline(device->getHandle().as<VkDevice>(), pipeline, nullptr);
    });

    descriptor_set.reset();

    if (layout)
    {
        vkDestroyPipelineLayout(device->getHandle().as<VkDevice>(), static_cast<VkPipelineLayout>(layout), nullptr);
        layout = nullptr;
    }
}

PipelineBuilder PipelineBuilder::fromLua(const std::string& source_dir, const std::string& script)
{
    SL::Runtime runtime(source_dir + script);
    const auto res = runtime.getGlobal<SL::Table>("Pipeline");
    MIDNIGHT_ASSERT(res, "Error loading pipeline from lua script: Global not found");

    PipelineBuilder builder;

    res->try_get<SL::Table>("shaders", [&](const SL::Table& shaders)
    {
        shaders.try_get<SL::String>("vertex",   [&](const SL::String& dir) { builder.addShader(source_dir + dir, ShaderType::Vertex);   });
        shaders.try_get<SL::String>("fragment", [&](const SL::String& dir) { builder.addShader(source_dir + dir, ShaderType::Fragment); });
    });

    res->try_get<SL::Number>("colorFormat", [&](const SL::Number& format){ builder.setColorFormat(static_cast<uint32_t>(format)); });
    res->try_get<SL::Number>("depthFormat", [&](const SL::Number& format){ builder.setDepthFormat(static_cast<uint32_t>(format)); });
    res->try_get<SL::Boolean>("depthTesting",    [&](const SL::Boolean& _bool){ builder.setDepthTesting(_bool); });
    res->try_get<SL::Boolean>("backfaceCulling", [&](const SL::Boolean& _bool){ builder.setBackfaceCull(_bool); });

    return builder;
}

void Pipeline::bindDescriptorSet(uint32_t index, const std::unique_ptr<Backend::CommandBuffer>& cmd) const
{
    descriptor_set->bind(index, cmd, layout);
}

PipelineBuilder& PipelineBuilder::addShader(std::filesystem::path path, ShaderType type)
{
    MIDNIGHT_ASSERT(!modules.count(type), "Shader type already present in pipeline");
    modules.emplace(type, std::make_shared<Shader>(path, type));
    return *this;
}

PipelineBuilder& PipelineBuilder::addShader(const std::shared_ptr<Shader>& shader)
{
    MIDNIGHT_ASSERT(!modules.count(shader->getType()), "Shader type already present in pipeline");
    modules.emplace(shader->getType(), shader);
    return *this;
}

PipelineBuilder& PipelineBuilder::setPolyMode(Polygon p)
{
    poly = p;
    return *this;
}

PipelineBuilder& PipelineBuilder::setTopology(Topology t)
{
    top = t;
    return *this;
}

PipelineBuilder& PipelineBuilder::setBackfaceCull(bool cull)
{
    backface_cull = cull;
    return *this;
}

PipelineBuilder& PipelineBuilder::setSize(uint32_t w, uint32_t h)
{
    size = std::pair(w, h);
    return *this;
}

PipelineBuilder& PipelineBuilder::setBlending(bool blend)
{
    blending = blend;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthTesting(bool d)
{
    depth = d;
    return *this;    
}

PipelineBuilder& PipelineBuilder::setColorFormat(uint32_t c)
{
    color_format = c;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthFormat(uint32_t d)
{
    depth_format = d;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDescriptorCount(uint32_t c)
{
    desc_count = c;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDescriptorSize(uint32_t c)
{
    desc_size = c;
    return *this;
}

Pipeline PipelineBuilder::build() const
{
    // If fixed state, then we need to assert
    //MIDNIGHT_ASSERT(size.first * size.second > 0, "Error building pipeline: Extent has zero area");

    const auto [layout, desc_layouts] = [&]()
    {
        auto& device = Backend::Instance::get()->getDevice();

        std::vector<VkDescriptorSetLayout> layouts;
        
        if (modules.count(ShaderType::Vertex))
        {
            const auto& vertex = modules.at(ShaderType::Vertex);
            const auto& desc_set_info = vertex->getDescriptorSetInfo();
            layouts.reserve(desc_set_info.size());

            for (const auto& set : desc_set_info)
            {
                auto data = std::reinterpret_pointer_cast<DescriptorSetData>(set);

                layouts.emplace_back();
                const auto err = vkCreateDescriptorSetLayout(
                    device->getHandle().as<VkDevice>(),
                    &data->create_info,
                    nullptr,
                    &layouts.back()
                );
                MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating descriptor set layout #" << layouts.size() << ": " << err);
            }
        }

        VkPipelineLayoutCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };

        VkPipelineLayout layout;
        const auto err = vkCreatePipelineLayout(device->getHandle().as<VkDevice>(), &create_info, nullptr, &layout);
        MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating pipeline layout");
        return std::pair(layout, layouts);
    }();

    const auto stages = [&]()
    {
        std::vector<VkPipelineShaderStageCreateInfo> stages;
        stages.reserve(modules.size());
        for (const auto& p : modules)
        {
            VkShaderStageFlagBits stage;
            switch (p.first)
            {
            case ShaderType::Vertex:   stage = VK_SHADER_STAGE_VERTEX_BIT;   break;
            case ShaderType::Fragment: stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
            case ShaderType::Compute:  stage = VK_SHADER_STAGE_COMPUTE_BIT;  break;
            default: MIDNIGHT_ASSERT(false, "Shader type can not be 'none'");
            }

            stages.push_back(VkPipelineShaderStageCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .pName = "main",
                .flags = 0,
                .pSpecializationInfo = nullptr,
                .stage = stage,
                .module = p.second->getHandle().as<VkShaderModule>()
            });
        }

        return stages;
    }();

    VkPipelineTessellationStateCreateInfo tesselation_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = 0
    };

    VkPipelineColorBlendAttachmentState color_blend_attach = {
        .blendEnable = (blending ? VK_TRUE : VK_FALSE),
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attach,
        .blendConstants = { 0, 0, 0, 0 },
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY
    };

    std::cout << "DEPTH: " << depth << "\n";
    const auto depth_stencil = ( depth ? 
        []()
        {
            return VkPipelineDepthStencilStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext = nullptr,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                .depthCompareOp = VK_COMPARE_OP_LESS,
                .depthBoundsTestEnable = VK_TRUE,
                .minDepthBounds = 0.f,
                .maxDepthBounds = 1.f,
                .stencilTestEnable = VK_FALSE,
                .front = {}, .back = {}
            };
        }() : 
        []()
        {
            return VkPipelineDepthStencilStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext = nullptr,
                .depthTestEnable = VK_FALSE,
                .depthWriteEnable = VK_FALSE,
                .depthCompareOp = VK_COMPARE_OP_NEVER,
                .depthBoundsTestEnable = VK_FALSE,
                .minDepthBounds = 0.f,
                .maxDepthBounds = 1.f,
                .stencilTestEnable = VK_FALSE,
                .front = {}, .back = {}
            };
        }()
    );

    // Dynamic State
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(), 
        .flags = 0
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    const auto poly_mode = [](const auto& p)
    {
        switch (p)
        {
        case Polygon::Fill: return VK_POLYGON_MODE_FILL;
        default:            return VK_POLYGON_MODE_FILL;
        }
    }(poly);

    VkPipelineRasterizationStateCreateInfo raster = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = poly_mode,
        .lineWidth = 1.f,
        .cullMode = (backface_cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE),
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_TRUE,
        .depthBiasClamp = 0.f,  
        .depthBiasConstantFactor = 0.f,
        .depthBiasSlopeFactor = 0.f
    };

    // Set the input assembly 
    const auto prim_top = [](const auto& t)
    {
        switch (t)
        {
        case Topology::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default:                  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }(top);

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .primitiveRestartEnable = VK_FALSE,
        .topology = prim_top,
    };

    const auto c_format = static_cast<VkFormat>(color_format);
    const auto d_format = static_cast<VkFormat>(depth_format);
    VkPipelineRenderingCreateInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &c_format,
        .depthAttachmentFormat = d_format
    };

    //VkVertexInputAttributeDescription desc;
    //desc.format = VK_FORMAT_

    VkPipelineVertexInputStateCreateInfo input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    std::vector<VkVertexInputAttributeDescription> attribs;
    VkVertexInputBindingDescription binding;

    if (modules.count(ShaderType::Vertex))
    {
        const auto& shader = modules.at(ShaderType::Vertex);
        const auto& attributes = shader->getAttributes();  
        

        uint32_t offset = 0;
        uint32_t location = 0;
        for (const auto& attrib : attributes)
        {
            attribs.push_back(VkVertexInputAttributeDescription {
                .binding  = static_cast<uint32_t>(attrib.binding),
                .format   = static_cast<VkFormat>(attrib.format),
                .location = location,
                .offset   = offset
            });
            location++;
            offset += attrib.element_size * attrib.element_count;
        }

        input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
        input_state.pVertexAttributeDescriptions = attribs.data();

        binding = VkVertexInputBindingDescription {
            .binding = 0,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            .stride = offset
        };

        input_state.vertexBindingDescriptionCount = 1;
        input_state.pVertexBindingDescriptions = &binding;
    }

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .scissorCount = 1,
        .viewportCount = 1
    };

    // construct all the graphics pipeline objects
    VkGraphicsPipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &render_info, // This should be a pointer to render_info
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = 0,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .layout = layout,
        .pTessellationState  = &tesselation_state,
        .pColorBlendState    = &color_blend,
        .pDepthStencilState  = &depth_stencil,
        .pDynamicState       = &dynamic_state,
        .pMultisampleState   = &multisample,
        .pRasterizationState = &raster,
        .pInputAssemblyState = &input_assembly,
        .pVertexInputState   = &input_state,
        .pViewportState      = &viewport_state
    };

    // actually build the graphics pipeline
    auto& device = Backend::Instance::get()->getDevice();
    
    VkPipeline pipeline;
    const auto err = vkCreateGraphicsPipelines(device->getHandle().as<VkDevice>(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating graphics pipeline: " << string_VkResult(err));

    Pipeline p(pipeline);
    p.binding_strides.push_back(binding.stride);
    p.layout = layout;

    std::vector<mn::handle_t> desc_layout_handles;
    for (const auto& layout : desc_layouts)
        desc_layout_handles.push_back( static_cast<mn::handle_t>(layout) );
    p.descriptor_set = std::make_unique<DescriptorSet>(desc_layout_handles, desc_count, desc_size);

    return p;
}

}
