#include <Graphics/Image.hpp>
#include <Graphics/Window.hpp>

#include <backends/imgui_impl_vulkan.h>

#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Device.hpp>

namespace mn::Graphics
{
    template<Image::Type T>
    Image::Attachment make_attachment(u32 format, Math::Vec2u size)
    {
        constexpr bool depth = (T == Image::DepthStencil);
        auto& device = Backend::Instance::get()->getDevice();
        Image::Attachment a;
        std::tie(a.handle, a.allocation) = 
            device->createImage(size, format, depth);
        a.view = device->createImageView(a.handle, format, depth);
        a.format = format;
        a.size = size; 
        
        if (Window::ImGui_Initialized)
            a.imgui_ds = ImGui_ImplVulkan_AddTexture(
                static_cast<VkSampler>(device->getSampler(Backend::Sampler::Nearest)->handle), 
                static_cast<VkImageView>(a.view), 
                VK_IMAGE_LAYOUT_GENERAL);
        else
            a.imgui_ds = nullptr;

        return a;
    }

    void Image::Attachment::destroy()
    {
        auto& device = Backend::Instance::get()->getDevice();
        device->destroyImageView(view);
        if (allocation)
            device->destroyImage(handle, allocation);
        
        // Why don't we need this (?)
        //if (imgui_ds)
            //ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(imgui_ds));

        view = nullptr;
        allocation = nullptr;
        handle = nullptr;
        imgui_ds = nullptr;
    }

    template<Image::Type T>
    void Image::Attachment::rebuild(u32 format, Math::Vec2u size)
    {
        destroy();
        auto& device = Backend::Instance::get()->getDevice();
        auto a = make_attachment<T>(format, size);

        allocation = a.allocation;
        view       = a.view;
        handle     = a.handle;
        format     = a.format;
        this->size = a.size;
        imgui_ds   = a.imgui_ds;
    }
    template void Image::Attachment::rebuild<Image::Color>(u32, Math::Vec2u);
    template void Image::Attachment::rebuild<Image::DepthStencil>(u32, Math::Vec2u);

    Image::~Image()
    {
        auto& device = Backend::Instance::get()->getDevice();

        if (depth_attachment) color_attachments.push_back(*depth_attachment);
        for (auto& a : color_attachments)
            a.destroy();
    }

    ImageFactory::ImageFactory() :
        image(std::unique_ptr<Image>(new Image()))
    {   }

    template<Image::Type T>
    ImageFactory& 
    ImageFactory::addAttachment(u32 format, const Math::Vec2u& size)
    {
        auto a = make_attachment<T>(format, size);
        if constexpr (T == Image::Type::Color)
            image->color_attachments.push_back(a);
        else
            image->depth_attachment.emplace(a);
        return *this;
    }
    template ImageFactory& ImageFactory::addAttachment<Image::Color>(u32, const Math::Vec2u&);
    template ImageFactory& ImageFactory::addAttachment<Image::DepthStencil>(u32, const Math::Vec2u&);

    template<Image::Type T>
    ImageFactory&
    ImageFactory::addImage(handle_t handle, u32 format, const Math::Vec2u& size)
    {
        constexpr bool depth = (T == Image::DepthStencil);
        auto& device = Backend::Instance::get()->getDevice();
        Image::Attachment a;
        a.handle = handle;
        a.allocation = nullptr;
        a.view = device->createImageView(a.handle, format, depth);
        a.format = format;
        a.size = size;

        if constexpr (T == Image::Type::Color)
            image->color_attachments.push_back(a);
        else
            image->depth_attachment.emplace(a);

        return *this;
    }
    template ImageFactory& ImageFactory::addImage<Image::Color>(handle_t, u32, const Math::Vec2u&);
    template ImageFactory& ImageFactory::addImage<Image::DepthStencil>(handle_t, u32, const Math::Vec2u&);
    
    Image&& 
    ImageFactory::build()
    {
        return std::move(*image.release());
    }
}