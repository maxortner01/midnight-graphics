#pragma once

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Image : ObjectHandle<Image>
    {
        Image(Handle<Image> h, bool depth = true);
        ~Image();

        auto getColorImageView() const { return color_view; }

    private:
        Handle<Image> color_view, depth_view;
    };
}