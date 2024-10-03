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
        };

        enum Format : u32
        {
            DF32_SU8 = 130
        };

        Image(const Image&) = delete;
        Image(Image&&) = default;

        ~Image();

        template<Type T>
        bool
        hasAttachment() const
        {
            return attachments.count(T);
        }

        template<Type T>
        const Attachment& 
        getAttachment() const
        {
            return attachments.at(T);
        }

    private:
        friend struct ImageFactory;

        Image() = default;

        std::unordered_map<Type, Attachment> attachments;
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