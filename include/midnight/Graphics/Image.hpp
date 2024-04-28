#pragma once

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Image : ObjectHandle<Image>
    {
        Image(Handle<Image> h, uint32_t f, std::pair<uint32_t, uint32_t> s, bool depth = true);
        ~Image();

        auto size()   const { return _size;   }
        auto format() const { return _format; }

        auto getColorImageView() const { return color_view; }
        auto getDepthImageView() const { return depth_view; }
        auto getDepthImage() const { return depth_image; }

    private:
        mn::handle_t depth_allocation;
        Handle<Image> color_view, depth_view, depth_image;
        uint32_t _format;
        std::pair<uint32_t, uint32_t> _size;
    };
}