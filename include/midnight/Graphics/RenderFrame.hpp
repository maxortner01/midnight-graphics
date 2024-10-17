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
    struct Descriptor;

    // [x] Remove any `const T&` arguments, replace with std::shared_ptr<T>
    // [x] Store the passed in pointers in an std::vector<std::shared_ptr<void>> in the frame_data
    // [x] Add `.release()` function to frame_data that clears this vector (call it from the Window::startFrame method)
    // [x] Add multiple color attachments to image
    // [ ] Pass optional image into startRender() (no passed in image defaults to given)
    // [x] Add explicit binding methods
    // [x] Require descriptor set std::shared_ptr in pipeline

    struct RenderFrame
    {
        friend struct Window;

        const uint32_t image_index;
        std::shared_ptr<Image> image;

        MN_SYMBOL void startRender(std::optional<std::shared_ptr<Image>> image = std::nullopt);
        MN_SYMBOL void endRender();

        MN_SYMBOL void clear(std::tuple<float, float, float> color, float alpha = 1.f, std::optional<std::shared_ptr<Image>> image = std::nullopt) const;
        
        MN_SYMBOL void setPushConstant(const Pipeline& pipeline, const void* data) const;
        template<typename T>
        void setPushConstant(const Pipeline& pipeline, const T& value) const
        {
            setPushConstant(pipeline, reinterpret_cast<const void*>(&value));
        }

        MN_SYMBOL void blit(const Image::Attachment& source, const Image::Attachment& destination) const;

        MN_SYMBOL void bind(const std::shared_ptr<Pipeline>& pipeline) const;

        // Does not bind pipeline
        MN_SYMBOL void bind(uint32_t set_index, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Descriptor>& descriptor) const;

        MN_SYMBOL void draw(uint32_t vertices, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const std::shared_ptr<Buffer>& buffer, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const std::shared_ptr<Mesh>& mesh, uint32_t instances = 1) const;
        MN_SYMBOL void drawIndexed(
            const std::shared_ptr<Buffer>& buffer, 
            const std::shared_ptr<TypeBuffer<uint32_t>>& indices, 
            uint32_t instances = 1,
            uint32_t index_offset = 0,
            std::optional<std::size_t> index_count = std::nullopt) const;

        MN_SYMBOL void draw(const std::shared_ptr<Pipeline>& pipeline, uint32_t vertices, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Buffer>& buffer, uint32_t instances = 1) const;
        MN_SYMBOL void draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Mesh>& mesh, uint32_t instances = 1) const;

        MN_SYMBOL void drawIndexed(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Buffer>& buffer, const std::shared_ptr<TypeBuffer<uint32_t>>& indices, uint32_t instances = 1) const;

    private:
        RenderFrame(uint32_t i, std::shared_ptr<Image> im) : image_index(i), image(im) { }

        std::shared_ptr<FrameData> frame_data;
    };
}