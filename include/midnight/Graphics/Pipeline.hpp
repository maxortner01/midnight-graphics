#pragma once

#include <vector>
#include <filesystem>

namespace mn::Graphics
{
    enum class ShaderType
    {
        Vertex, Fragment, Compute
    };

    struct Shader
    {
        Shader();
        Shader(std::filesystem::path path, ShaderType type);

        void fromFile(std::filesystem::path path, ShaderType type);
        void fromString(const std::string& contents, ShaderType type, const std::string& path = "");

        const std::vector<uint32_t>& getData() const { return data; }
    
    private:
        std::vector<uint32_t> data;
    };
}