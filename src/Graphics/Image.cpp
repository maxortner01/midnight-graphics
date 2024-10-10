#include <Graphics/Image.hpp>

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
        return a;
    }

    Image::~Image()
    {
        auto& device = Backend::Instance::get()->getDevice();
        for (const auto& [ _, a ] : attachments)
        {
            device->destroyImageView(a.view);
            if (a.allocation)
                device->destroyImage(a.handle, a.allocation);
        }
    }

    template<Image::Type T>
    void Image::rebuildAttachment(u32 format, Math::Vec2u size)
    {
        auto& device = Backend::Instance::get()->getDevice();
        attachments.erase(T);
        attachments[T] = make_attachment<T>(format, size);
    }
    template void Image::rebuildAttachment<Image::Type::Color>(u32, Math::Vec2u);
    template void Image::rebuildAttachment<Image::Type::DepthStencil>(u32, Math::Vec2u);

    ImageFactory::ImageFactory() :
        image(std::unique_ptr<Image>(new Image()))
    {   }

    template<Image::Type T>
    ImageFactory& 
    ImageFactory::addAttachment(u32 format, const Math::Vec2u& size)
    {
        image->attachments[T] = make_attachment<T>(format, size);
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

        image->attachments[T] = std::move(a);

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