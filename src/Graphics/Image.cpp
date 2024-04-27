#include <Graphics/Image.hpp>

#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Device.hpp>

namespace mn::Graphics
{
    Image::Image(Handle<Image> h, uint32_t f, std::pair<uint32_t, uint32_t> s, bool depth) :
        ObjectHandle(h),
        _format(f),
        _size(s)
    {   
        auto& device = Backend::Instance::get()->getDevice();
        color_view = device->createImageView(h);
    }

    Image::~Image()
    {
        auto& device = Backend::Instance::get()->getDevice();
        if (color_view)
        {
            device->destroyImageView(color_view);
            color_view = nullptr;
        }

        if (depth_view)
        {
            depth_view = nullptr;
        }
    }
}