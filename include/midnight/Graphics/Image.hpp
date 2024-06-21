#pragma once

#include <Math.hpp>

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Image : ObjectHandle<Image>
    {
        MN_SYMBOL Image(u32 format, const Math::Vec2u& size, bool depth = true);
        MN_SYMBOL Image(Handle<Image> h, u32 f, const Math::Vec2u& s, bool depth = true);
        MN_SYMBOL ~Image();

        auto size()   const { return _size;   }
        auto format() const { return _format; }

        auto getColorImageView() const { return color_view; }
        auto getDepthImageView() const { return depth_view; }
        auto getDepthImage() const { return depth_image; }

    private:
        mn::handle_t image_allocation, depth_allocation;
        Handle<Image> color_view, depth_view, depth_image;
        u32 _format;
        Math::Vec2u _size;
    };
}