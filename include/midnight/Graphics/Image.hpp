#pragma once

#include <Math.hpp>

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    // Reformat this as image w/ attachments
    // Then we can add has_attachment<>() function to determine if there's depth/stencil
    struct ImageFactory;

    struct Image
    {
        enum Type
        {
            Color, DepthStencil
        };

        struct Attachment
        {
            mn::handle_t handle, allocation, view;
            u32 format;
            Math::Vec2u size;

            // IF IMGUI
            mn::handle_t imgui_ds;

            void destroy();

            template<Type T>
            void rebuild(u32 format, Math::Vec2u size);
        };

        enum Format : u32
        {
            DF32_SU8 = 130,
            R8G8B8A8_UNORM = 37,
            B8G8R8A8_UNORM = 44,
            R16G16B16A16_SFLOAT = 97
        };

        Image(const Image&) = delete;
        Image(Image&&) = default;

        ~Image();

        bool hasDepthAttachment() const
        {
            return depth_attachment.has_value();
        }

        const Attachment&
        getDepthAttachment() const
        {
            return *depth_attachment;
        }

        Attachment&
        getDepthAttachment()
        {
            return *depth_attachment;
        }

        const auto&
        getColorAttachments() const
        {
            return color_attachments;
        }

        auto&
        getColorAttachments()
        {
            return color_attachments;
        }

    private:
        friend struct ImageFactory;

        Image() = default;

        std::vector<Attachment> color_attachments;
        std::optional<Attachment> depth_attachment;
    };

    struct ImageFactory
    {
        ImageFactory();

        template<Image::Type T>
        ImageFactory&
        addAttachment(u32 format, const Math::Vec2u& size);

        template<Image::Type T>
        ImageFactory&
        addImage(handle_t handle, u32 format, const Math::Vec2u& size);

        [[nodiscard]] Image&& 
        build();
    
    private:
        std::unique_ptr<Image> image;
    };
}