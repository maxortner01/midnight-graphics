#pragma once

#include "Image.hpp"

#include <filesystem>

namespace mn::Graphics
{
    struct Texture
    {
        Texture() = default;
        Texture(const std::filesystem::path& filepath);
        Texture(const Texture&) = delete;

        void loadFromFile(const std::filesystem::path& filepath);

        std::shared_ptr<Image> get_image() const { return image; }

    private:
        std::shared_ptr<Image> image;
    };
}