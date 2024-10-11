#include <midnight/Graphics/Backend/Instance.hpp>
#include <midnight/Graphics/Backend/Command.hpp>
#include <midnight/Graphics/Texture.hpp>
#include <midnight/Graphics/Buffer.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mn::Graphics
{
    Texture::Texture(const std::filesystem::path& filepath)
    {
        loadFromFile(filepath);
    }

    void Texture::loadFromFile(const std::filesystem::path& filepath)
    {
        MIDNIGHT_ASSERT(!image, "Texture already loaded!");

        int x, y, z, n;
        uint8_t* data = stbi_load(filepath.c_str(), &x, &y, &z, 4);
        MIDNIGHT_ASSERT(data, "Error loading texture data from path: " << filepath.string());

        // Allocate GPU data for the image
        image = std::make_shared<mn::Graphics::Image>(
            mn::Graphics::ImageFactory()
                .addAttachment<mn::Graphics::Image::Color>(
                    mn::Graphics::Image::R8G8B8A8_UNORM, 
                    { static_cast<uint32_t>(x), static_cast<uint32_t>(y) })
                .build()
        );

        // Allocate the GPU buffer "staging" zone
        auto image_data = std::make_shared<mn::Graphics::TypeBuffer<uint8_t>>();
        image_data->resize(x * y * 4);

        // Copy the CPU data into the GPU region
        std::memcpy(&image_data->at(0), data, x * y * 4);

        // Free the CPU data
        STBI_FREE(data);

        // Copy the staging data into the GPU image data
        auto& device = Backend::Instance::get()->getDevice();
        device->immediateSubmit([this, &image_data](mn::Graphics::Backend::CommandBuffer& cmd)
        {
            cmd.bufferToImage(image_data, image->getColorAttachments()[0]);
        });
    }
}