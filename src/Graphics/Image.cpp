#include <Graphics/Image.hpp>

#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Device.hpp>

namespace mn::Graphics
{
    Image::Image(Handle<Image> h, uint32_t f, std::pair<uint32_t, uint32_t> s, bool depth) :
        ObjectHandle(h),
        _format(f),
        _size(s),
        depth_allocation{0}
    {   
        auto& device = Backend::Instance::get()->getDevice();
        color_view = device->createImageView(h, f);

        if (depth)
        {
            std::tie(depth_image, depth_allocation) = device->createImage(Math::Vec2u({ s.first, s.second }), 130, true);
            depth_view = device->createImageView(depth_image, 130, true);
        }
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
            device->destroyImageView(depth_view);
            depth_view = nullptr;
        }

        if (depth_image)
        {
            device->destroyImage(depth_image, depth_allocation);
            depth_image = nullptr;
        }
    }
}