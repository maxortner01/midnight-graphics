#pragma once

#include <Def.hpp>

#include "Model.hpp"
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
        std::stack<std::shared_ptr<const Image>> image_stack;
        std::shared_ptr<const Image> image;

        MN_SYMBOL void startRender();
        MN_SYMBOL void endRender();

        MN_SYMBOL void clear(std::tuple<float, float, float> color) const;
        
        MN_SYMBOL void setPushConstant(const Pipeline& pipeline, const void* data) const;
        template<typename T>
        void setPushConstant(const Pipeline& pipeline, const T& value) const
        {
            setPushConstant(pipeline, reinterpret_cast<const void*>(&value));
        }

        MN_SYMBOL void blit(std::shared_ptr<const Image> source, std::shared_ptr<const Image> destination) const;

        MN_SYMBOL void draw(const Pipeline& pipeline, uint32_t vertices, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const Pipeline& pipeline, const Buffer& buffer, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const Pipeline& pipeline, const Model& model, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const Pipeline& pipeline, std::shared_ptr<Model> model, uint32_t instances = 1) const;

        MN_SYMBOL void drawIndexed(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer, std::shared_ptr<Buffer> indices, uint32_t instances = 1) const;

    private:
        std::shared_ptr<const Image> get_image() const
        {
            return (image_stack.size() ? image_stack.top() : image);
        }

        RenderFrame(uint32_t i, std::shared_ptr<Image> im) : image_index(i), image(im) { }

        std::shared_ptr<FrameData> frame_data;
    };
}