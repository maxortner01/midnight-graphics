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

    Compiler compiler;
    CompileOptions options;
    const auto result = compiler.CompileGlslToSpv(contents, shaderc_vertex_shader, path.c_str());
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

}