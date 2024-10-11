#pragma once

#include <Def.hpp>

#include "Mesh.hpp"
#include "Buffer.hpp"
#include "Image.hpp"

namespace mn::Graphics
{
    struct Window;
    struct FrameData;
    struct Pipeline;

    // [x] Remove any `const T&` arguments, replace with std::shared_ptr<T>
    // [x] Store the passed in pointers in an std::vector<std::shared_ptr<void>> in the frame_data
    // [x] Add `.release()` function to frame_data that clears this vector (call it from the Window::startFrame method)
    // [ ] Add multiple color attachments to image
    // [ ] Pass optional image into startRender() (no passed in image defaults to given)
    // [x] Add explicit binding methods
    // [x] Require descriptor set std::shared_ptr in pipeline

    struct RenderFrame
    {
        friend struct Window;

        const uint32_t image_index;
        std::stack<std::shared_ptr<Image>> image_stack;
        std::shared_ptr<Image> image;

        // The images are possible extra render attachments
        MN_SYMBOL void startRender(std::optional<std::vector<std::shared_ptr<Image>>> images = std::nullopt);
        MN_SYMBOL void endRender();

        MN_SYMBOL void clear(std::tuple<float, float, float> color, float alpha = 1.f) const;
        
        MN_SYMBOL void setPushConstant(const Pipeline& pipeline, const void* data) const;
        template<typename T>
        void setPushConstant(const Pipeline& pipeline, const T& value) const
        {
            setPushConstant(pipeline, reinterpret_cast<const void*>(&value));
        }

        MN_SYMBOL void blit(std::shared_ptr<const Image> source, std::shared_ptr<const Image> destination) const;

        MN_SYMBOL void bind(const std::shared_ptr<Pipeline>& pipeline) const;

        MN_SYMBOL void draw(const std::shared_ptr<Pipeline>& pipeline, uint32_t vertices, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Buffer>& buffer, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Mesh>& mesh, uint32_t instances = 1) const;

        MN_SYMBOL void drawIndexed(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Buffer>& buffer, const std::shared_ptr<Buffer>& indices, uint32_t instances = 1) const;

    private:
        std::shared_ptr<Image> get_image() const
        {
            return (image_stack.size() ? image_stack.top() : image);
        }

        RenderFrame(uint32_t i, std::shared_ptr<Image> im) : image_index(i), image(im) { }

        std::shared_ptr<FrameData> frame_data;
    };
}