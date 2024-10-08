#include <Graphics/Image.hpp>

#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Device.hpp>

namespace mn::Graphics
{
    Image::~Image()
    {
        auto& device = Backend::Instance::get()->getDevice();
        for (auto& [ type, a ] : attachments)
        {
            device->destroyImageView(a.view);

            if (a.allocation)
                device->destroyImage(a.handle, a.allocation);
        }
    }

    ImageFactory::ImageFactory() :
        image(std::unique_ptr<Image>(new Image()))
    {   }

    template<Image::Type T>
    ImageFactory& 
    ImageFactory::addAttachment(u32 format, const Math::Vec2u& size)
    {
        constexpr bool depth = (T == Image::DepthStencil);
        auto& device = Backend::Instance::get()->getDevice();
        Image::Attachment a;
        std::tie(a.handle, a.allocation) = 
            device->createImage(size, format, depth);
        a.view = device->createImageView(a.handle, format, depth);
        a.format = format;
        a.size = size; 

        image->attachments[T] = std::move(a);

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