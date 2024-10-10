#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Pipeline.hpp>
#include <Graphics/Buffer.hpp>
#include <Graphics/Image.hpp>
#include <Graphics/Backend/Command.hpp>

#include <set>
#include <Def.hpp>

#include <algorithm>
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

        // this is a crappy way of doing this
        std::erase_if(vars, [](const auto* var) { return (std::string(var->name).find("gl_VertexIndex") != std::string::npos); });
        std::erase_if(vars, [](const auto* var) { return (std::string(var->name).find("gl_InstanceIndex") != std::string::npos); });
        std::sort(vars.begin(), vars.end(), [](auto*& a, auto*& b) { return a->location < b->location; });

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
                .element_size = sizeof(float),
                .format = static_cast<uint32_t>(var->format),
                .binding = 0
            });
        }

        spvReflectDestroyShaderModule(&shader_mod);
    }
}

/*

DescriptorSet::~DescriptorSet()
{
    auto& device = Backend::Instance::get()->getDevice();
    vkDestroyDescriptorPool(device->getHandle().as<VkDevice>(), static_cast<VkDescriptorPool>(pool), nullptr);
    vkDestroyDescriptorSetLayout(device->getHandle().as<VkDevice>(), static_cast<VkDescriptorSetLayout>(layout), nullptr);
}

void DescriptorSet::setImages(uint32_t binding, Backend::Sampler::Type type, const std::vector<std::shared_ptr<Image>>& images)
{
    auto& device = Backend::Instance::get()->getDevice();
    auto sampler = device->getSampler(type);

    std::vector<VkDescriptorImageInfo> infos;
    infos.reserve(images.size());
    for (const auto& image : images)
    {
        const auto& color = image->getAttachment<Image::Color>();
        infos.push_back(VkDescriptorImageInfo{
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = static_cast<VkImageView>(color.view),
            .sampler = static_cast<VkSampler>(sampler->handle)
        });
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = infos.size();
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstSet = static_cast<VkDescriptorSet>(handle);
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.pImageInfo = infos.data();

    vkUpdateDescriptorSets(device->getHandle().as<VkDevice>(), 1, &write, 0, nullptr);
}*/

Pipeline::~Pipeline()
{
    auto& device = Backend::Instance::get()->getDevice();

    handle.destroy<VkPipeline>([&device](VkPipeline pipeline)
    {
        vkDestroyPipeline(device->getHandle().as<VkDevice>(), pipeline, nullptr);
    });

    if (layout)
    {
        vkDestroyPipelineLayout(device->getHandle().as<VkDevice>(), static_cast<VkPipelineLayout>(layout), nullptr);
        layout = nullptr;
    }
}
// TODO: Need to be able to specify vertex or fragment
void Pipeline::setPushConstant(const std::unique_ptr<Backend::CommandBuffer>& cmd, const void* data) const
{
    vkCmdPushConstants(cmd->getHandle().as<VkCommandBuffer>(), static_cast<VkPipelineLayout>(layout), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, push_constant_size, data);
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
    res->try_get<SL::Number>("polygon", [&](const SL::Number& polygon){ builder.setPolyMode(static_cast<Polygon>(polygon)); });

    return builder;
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

PipelineBuilder& PipelineBuilder::addSet(Descriptor& d)
{
    setLayouts.push_back(d.getLayoutHandle());
    sets.push_back(d.getHandle());
    return *this;
}

PipelineBuilder& PipelineBuilder::setCullDirection(bool clockwise)
{
    this->clockwise = clockwise;
    return *this;
}

Pipeline PipelineBuilder::build() const
{
    // If fixed state, then we need to assert
    //MIDNIGHT_ASSERT(size.first * size.second > 0, "Error building pipeline: Extent has zero area");
    
    //std::unique_ptr<DescriptorSet> desc;

    const auto layout = [&]()
    {
        auto& device = Backend::Instance::get()->getDevice();
        /*
        // Construct the set here if necessary
        if (bindings.size())
        {
            //const std::unordered_map<VkDescriptorType, uint32_t> sizes = {
                //{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 640 },
                //{ VK_DESCRIPTOR_TYPE_SAMPLER, 4 }
            //};

            std::unordered_map<VkDescriptorType, uint32_t> types;
            const auto get_type = [&](Binding::Type t)
            {
                switch (t)
                {
                case Binding::Type::Texture: 
                    types[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]++;
                    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                }
            };

            // We need two bindings
            // One binding for textures
            // Another for samplers, we really only have the two samplers from the device

            // Hardcoded for now. We add a variable descriptor for the textures and a size 2 descriptor for samplers

            std::vector<VkDescriptorSetLayoutBinding> _bindings;
            std::vector<VkDescriptorBindingFlagsEXT> _binding_flags;
            std::vector<uint32_t> counts(bindings.size(), 16);
            for (uint32_t i = 0; i < bindings.size(); i++)
            {
                const auto binding_type = get_type(bindings[i].type);
                _bindings.push_back(VkDescriptorSetLayoutBinding {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .binding = i,
                    .descriptorCount = 16,
                    .descriptorType = binding_type
                });

                _binding_flags.push_back(
                    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
                    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                    VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
                    VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT);
            }

            VkDescriptorSetLayoutCreateInfo layout_create_info;
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.bindingCount = _bindings.size();
            layout_create_info.pBindings = _bindings.data();
            layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
            binding_flags.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            binding_flags.bindingCount   = _binding_flags.size();
            binding_flags.pBindingFlags  = _binding_flags.data();
            layout_create_info.pNext = &binding_flags;

            VkDescriptorSetLayout desc_layout;
            MIDNIGHT_ASSERT(vkCreateDescriptorSetLayout(
                device->getHandle().as<VkDevice>(), 
                &layout_create_info, 
                nullptr, 
                &desc_layout) == VK_SUCCESS, "Error creating descriptor layout");
            
            std::vector<VkDescriptorPoolSize> pool_sizes;
            for (const auto& [type, count] : types)
                pool_sizes.push_back(VkDescriptorPoolSize {
                    .type = type,
                    .descriptorCount = 16 
                });

            VkDescriptorPoolCreateInfo pool_create_info{};
            pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_create_info.pNext = nullptr;
            pool_create_info.poolSizeCount = pool_sizes.size();
            pool_create_info.pPoolSizes = pool_sizes.data();
            pool_create_info.maxSets = 1;
            pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

            VkDescriptorPool pool;
            MIDNIGHT_ASSERT(vkCreateDescriptorPool(
                device->getHandle().as<VkDevice>(), 
                &pool_create_info, 
                nullptr, 
                &pool) == VK_SUCCESS, "Failed to create descriptor pool");

            VkDescriptorSetVariableDescriptorCountAllocateInfo variable_alloc{};
            variable_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            variable_alloc.pNext = nullptr;
            variable_alloc.descriptorSetCount = counts.size();
            variable_alloc.pDescriptorCounts = counts.data();

            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.pNext = &variable_alloc;
            alloc_info.descriptorPool = pool;
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &desc_layout;

            VkDescriptorSet set;
            MIDNIGHT_ASSERT(vkAllocateDescriptorSets(
                device->getHandle().as<VkDevice>(), 
                &alloc_info, 
                &set) == VK_SUCCESS, "Failed to create set");
            
            desc = std::unique_ptr<DescriptorSet>(new DescriptorSet(set));
            desc->pool = pool;
            desc->layout = desc_layout;
        }*/

        // TODO: Need to be able to specify vertex and/or fragment
        VkPushConstantRange push_constant = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = push_constant_size
        };

        VkPipelineLayoutCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
            .pSetLayouts = reinterpret_cast<const VkDescriptorSetLayout*>(setLayouts.data()),
            .pushConstantRangeCount = ( push_constant_size ? 1U : 0U ),
            .pPushConstantRanges = &push_constant
        };

        VkPipelineLayout layout;
        const auto err = vkCreatePipelineLayout(device->getHandle().as<VkDevice>(), &create_info, nullptr, &layout);
        MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating pipeline layout");
        return layout;
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
                .flags = 0,
                .stage = stage,
                .module = p.second->getHandle().as<VkShaderModule>(),
                .pName = "main",
                .pSpecializationInfo = nullptr
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
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attach,
        .blendConstants = { 0, 0, 0, 0 }
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
                .stencilTestEnable = VK_FALSE,
                .front = {}, .back = {},
                .minDepthBounds = 0.f,
                .maxDepthBounds = 1.f
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
                .stencilTestEnable = VK_FALSE,
                .front = {}, .back = {},
                .minDepthBounds = 0.f,
                .maxDepthBounds = 1.f
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
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    const auto poly_mode = [](const auto& p)
    {
        switch (p)
        {
        case Polygon::Fill:      return VK_POLYGON_MODE_FILL;
        case Polygon::Wireframe: return VK_POLYGON_MODE_LINE;
        default:                 return VK_POLYGON_MODE_FILL;
        }
    }(poly);

    VkPipelineRasterizationStateCreateInfo raster = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = poly_mode,
        .cullMode = static_cast<VkCullModeFlags>(backface_cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE),
        .frontFace = (clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE),
        .depthBiasEnable = VK_TRUE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 0.f,  
        .depthBiasSlopeFactor = 0.f,
        .lineWidth = 1.f
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
        .topology = prim_top,
        .primitiveRestartEnable = VK_FALSE
    };

    const auto c_format = static_cast<VkFormat>(color_format);
    const auto d_format = static_cast<VkFormat>(depth_format);
    VkPipelineRenderingCreateInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &c_format,
        .depthAttachmentFormat = d_format,
        .stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT
    };

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
                .location = location,
                .binding  = static_cast<uint32_t>(attrib.binding),
                .format   = static_cast<VkFormat>(attrib.format),
                .offset   = offset
            });
            location++;
            offset += attrib.element_size * attrib.element_count;
        }

        input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
        input_state.pVertexAttributeDescriptions = ( attribs.size() ? attribs.data() : nullptr );

        binding = VkVertexInputBindingDescription {
            .binding = 0,
            .stride = offset,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        input_state.vertexBindingDescriptionCount = ( attribs.size() ? 1 : 0 );
        input_state.pVertexBindingDescriptions = &binding;
    }

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // construct all the graphics pipeline objects
    VkGraphicsPipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &render_info, // This should be a pointer to render_info
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState   = &input_state,
        .pInputAssemblyState = &input_assembly,
        .pTessellationState  = &tesselation_state,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &raster,
        .pMultisampleState   = &multisample,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blend,
        .pDynamicState       = &dynamic_state,
        .layout = layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = 0
    };

    // actually build the graphics pipeline
    auto& device = Backend::Instance::get()->getDevice();
    
    VkPipeline pipeline;
    const auto err = vkCreateGraphicsPipelines(device->getHandle().as<VkDevice>(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating graphics pipeline: " << string_VkResult(err));

    auto p = std::unique_ptr<Pipeline>(new Pipeline(pipeline));
    p->binding_strides.push_back(binding.stride);
    p->layout = layout;
    p->push_constant_size = push_constant_size;
    p->sets = sets;
    //p->set = std::move(desc);

    return std::move(*p.release());
}

}
