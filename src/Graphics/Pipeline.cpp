#include <Graphics/Pipeline.hpp>
#include <Def.hpp>

#include <iostream>
#include <cassert>
#include <exception>
#include <sstream>
#include <fstream>

#include <shaderc/shaderc.hpp>

namespace mn::Graphics
{

Shader::Shader(std::filesystem::path path, ShaderType type)
{
    fromFile(path, type);
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

    data.reserve(result.cend() - result.cbegin());
    for (const auto* it = result.cbegin(); it != result.cend(); it++)
        data.push_back(*it);
}

}