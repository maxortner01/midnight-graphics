#pragma once

#include <Def.hpp>

#include "Buffer.hpp"
#include "Image.hpp"

namespace mn::Graphics
{
    struct Window;
    struct FrameData;
    struct Pipeline;

    struct RenderFrame
    {
        friend struct Window;

        const uint32_t image_index;
        std::shared_ptr<const Image> image;

        void startRender();
        void endRender();

        void clear(std::tuple<float, float, float> color) const;
        void draw(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer) const;

    private:
        RenderFrame(uint32_t i, std::shared_ptr<Image> im) : image_index(i), image(im) { }

        std::shared_ptr<FrameData> frame_data;
    };
}