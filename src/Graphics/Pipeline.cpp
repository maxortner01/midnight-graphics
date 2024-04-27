#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Pipeline.hpp>
#include <Def.hpp>

#include <iostream>
#include <cassert>
#include <exception>
#include <sstream>
#include <fstream>

#include <shaderc/shaderc.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

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
}

/*
void Pipeline::addShader(std::filesystem::path path, ShaderType type)
{
    MIDNIGHT_ASSERT(!modules.count(type), "Shader already created for this pipeline!");
    modules.insert(std::pair(type, std::make_unique<Shader>(path, type)));
}

void Pipeline::build()
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
            .pName = "",
            .flags = 0,
            .pSpecializationInfo = nullptr,
            .stage = stage,
            .module = p.second->handle.as<VkShaderModule>()
        });
    }
}
*/

PipelineLayout::PipelineLayout() : 
    ObjectHandle([]()
    {
        VkPipelineLayoutCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr
        };

        auto& device = Backend::Instance::get()->getDevice();

        VkPipelineLayout layout;
        const auto err = vkCreatePipelineLayout(device->getHandle().as<VkDevice>(), &create_info, nullptr, &layout);
        MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating pipeline layout");
        return layout;
    }())
{   }

PipelineLayout::~PipelineLayout()
{
    if (handle)
    {
        auto& device = Backend::Instance::get()->getDevice();
        vkDestroyPipelineLayout(device->getHandle().as<VkDevice>(), handle.as<VkPipelineLayout>(), nullptr);
        handle = nullptr;
    }
}

Pipeline::~Pipeline()
{
    if (handle)
    {
        auto& device = Backend::Instance::get()->getDevice();
        vkDestroyPipeline(device->getHandle().as<VkDevice>(), handle.as<VkPipeline>(), nullptr);
        handle = nullptr;
    }
}

PipelineBuilder& PipelineBuilder::addLayout(const std::shared_ptr<PipelineLayout>& layout)
{
    _layout = layout;
    return *this;
}

PipelineBuilder& PipelineBuilder::createLayout()
{
    _layout = std::make_shared<PipelineLayout>();
    return *this;
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

Pipeline PipelineBuilder::build() const
{
    // If fixed state, then we need to assert
    //MIDNIGHT_ASSERT(size.first * size.second > 0, "Error building pipeline: Extent has zero area");

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

    VkPipelineVertexInputStateCreateInfo input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr
    };

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
        .layout = _layout->getHandle().as<VkPipelineLayout>(),
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

    return Pipeline(pipeline);
}

}
