#pragma once

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

    struct Shader
    {
        friend struct Pipeline;

        Shader();
        Shader(std::filesystem::path path, ShaderType type);
        ~Shader();

        void fromFile(std::filesystem::path path, ShaderType type);
        void fromString(const std::string& contents, ShaderType type, const std::string& path = "");
        void fromSpv(const std::vector<uint32_t>& contents, ShaderType type);
    
    private:
        Handle<Shader> handle;
        ShaderType type;
    };

    struct Pipeline
    {
        void addShader(std::filesystem::path path, ShaderType type);

        void build();

    private:
        std::unordered_map<ShaderType, std::unique_ptr<Shader>> modules;
    };
}