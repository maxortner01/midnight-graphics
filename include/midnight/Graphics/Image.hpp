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

    private:
        Handle<Image> color_view, depth_view;
        uint32_t _format;
        std::pair<uint32_t, uint32_t> _size;
    };
}