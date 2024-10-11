#include <Graphics/Window.hpp>
#include <Graphics/RenderFrame.hpp>
#include <Graphics/Pipeline.hpp>

#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Device.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics
{

// Should make this a function in Backend::Device, it's copied from Window.cpp
void _transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkImageAspectFlags aspectMask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageMemoryBarrier2 image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .image = image,
        .subresourceRange = [](VkImageAspectFlags aspectMask)
            {
                return VkImageSubresourceRange {
                    .aspectMask = aspectMask,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS
                };
            }(aspectMask)
    };

    VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier
    };
    
    auto& device = Backend::Instance::get()->getDevice();
    PFN_vkVoidFunction pVkCmdPipelineBarrier2KHR = vkGetDeviceProcAddr(device->getHandle().as<VkDevice>(), "vkCmdPipelineBarrier2KHR");
    ((PFN_vkCmdPipelineBarrier2KHR)(pVkCmdPipelineBarrier2KHR ))(cmd, &dep_info);
};

void RenderFrame::startRender(std::optional<std::shared_ptr<Image>> image) // maybe we can pass in a std::vector of images, then we can add the attachments on
{
    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    const auto use_image = ( image ? *image : this->image);

    frame_data->resources.insert(use_image);
    std::vector<VkRenderingAttachmentInfo> attachments;
    const auto& color_attachments = use_image->getColorAttachments();
    for (const auto& a : color_attachments)
    {
        _transition_image(
            cmdBuffer, 
            static_cast<VkImage>(a.handle), 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_GENERAL
        );

        attachments.push_back(VkRenderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = static_cast<VkImageView>(a.view),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }

    std::optional<VkRenderingAttachmentInfo> depth_attach;

    if (use_image->hasDepthAttachment())
    {
        _transition_image(
            cmdBuffer, 
            static_cast<VkImage>(use_image->getDepthAttachment().handle), 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );

        auto attachment_info = VkRenderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = static_cast<VkImageView>( use_image->getDepthAttachment().view ),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .depthStencil = { .depth = 1 } }
        };

        depth_attach.emplace(attachment_info);
    }

    const auto& image_size = use_image->getColorAttachments()[0].size;
    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { 0, 0, Math::x( image_size ), Math::y( image_size ) },
        .layerCount = 1,
        .colorAttachmentCount = static_cast<uint32_t>(attachments.size()), // For a G-buffer, we can attach multiple color attachments
        .pColorAttachments = attachments.data(),
        .pDepthAttachment = ( depth_attach.has_value() ? &(*depth_attach) : nullptr )
    };

    const auto command_buffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    VkRect2D sc = { 0, 0, Math::x( image_size ), Math::y( image_size ) };
    vkCmdSetScissor(command_buffer, 0, 1, &sc);

    VkViewport extent = { .x = 0, .y = 0, .width = static_cast<float>(Math::x(image_size)), .height = static_cast<float>(Math::y(image_size)), .minDepth = 0, .maxDepth = 1.f };
    vkCmdSetViewport(command_buffer, 0, 1, &extent);

    auto& device = Backend::Instance::get()->getDevice();
    auto pvkCmdBeginRenderingKHR = vkGetDeviceProcAddr(device->getHandle().as<VkDevice>(), "vkCmdBeginRenderingKHR");
    ((PFN_vkCmdBeginRenderingKHR)(pvkCmdBeginRenderingKHR))(
        command_buffer,
        &render_info);
}

void RenderFrame::endRender()
{
    auto& device = Backend::Instance::get()->getDevice();

    auto pvkCmdEndRenderingKHR = vkGetDeviceProcAddr(device->getHandle().as<VkDevice>(), "vkCmdEndRenderingKHR");
    ((PFN_vkCmdEndRenderingKHR)(pvkCmdEndRenderingKHR))(frame_data->command_buffer->getHandle().as<VkCommandBuffer>());
}

void RenderFrame::clear(std::tuple<float, float, float> color, float alpha, std::optional<std::shared_ptr<Image>> image) const
{   
    const auto use_image = ( image ? *image : this->image );

    VkClearColorValue clearValue;
	clearValue = { { std::get<0>(color), std::get<1>(color), std::get<2>(color), alpha } };

    const auto aspect_bits = [](VkImageAspectFlags aspectMask)
    {
        return VkImageSubresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS
        };
    };

    const auto colorRange = aspect_bits(VK_IMAGE_ASPECT_COLOR_BIT);
    const auto dsRange = aspect_bits(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    VkClearDepthStencilValue depthvalue;
    depthvalue.depth = 0;
    depthvalue.stencil = 0;

    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    const auto& color_attachments = use_image->getColorAttachments();
    for (const auto& a : color_attachments)
    {
        _transition_image(
            cmdBuffer, 
            static_cast<VkImage>(a.handle), 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_GENERAL);

        //clear image
        vkCmdClearColorImage(
            frame_data->command_buffer->getHandle().as<VkCommandBuffer>(), 
            static_cast<VkImage>(a.handle), 
            VK_IMAGE_LAYOUT_GENERAL, 
            &clearValue, 
            1, 
            &colorRange);
    }
}

void RenderFrame::setPushConstant(const Pipeline& pipeline, const void* data) const
{
    pipeline.setPushConstant(frame_data->command_buffer, data);
}

void RenderFrame::blit(const Image::Attachment& source, const Image::Attachment& destination) const
{
    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    const auto& source_attachment      = source;
    const auto& destination_attachment = destination;

    _transition_image(cmdBuffer, static_cast<VkImage>(source_attachment.handle), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    _transition_image(cmdBuffer, static_cast<VkImage>(destination_attachment.handle), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkImageBlit blit;
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { 
        static_cast<int32_t>(Math::x(source_attachment.size)), 
        static_cast<int32_t>(Math::y(source_attachment.size)), 
        1 
    };

    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { 
        static_cast<int32_t>(Math::x(destination_attachment.size)), 
        static_cast<int32_t>(Math::y(destination_attachment.size)), 
        1 
    };

    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;

    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    // Need to set a memory barrier and transition the layouts here
    // Source needs to become layout_general
    // Destination needs to become layout_general

    vkCmdBlitImage(
        cmdBuffer,
        static_cast<VkImage>(source_attachment.handle),
        VK_IMAGE_LAYOUT_GENERAL,
        static_cast<VkImage>(destination_attachment.handle),
        VK_IMAGE_LAYOUT_GENERAL, //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        1,
        &blit,
        VkFilter::VK_FILTER_NEAREST
    );
}

void RenderFrame::bind(const std::shared_ptr<Pipeline>& pipeline) const
{
    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    frame_data->resources.insert(pipeline);
    vkCmdBindPipeline(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->getHandle().as<VkPipeline>());

    const auto& sets = pipeline->getDescriptors();
    if (sets.size())
    {
        std::vector<VkDescriptorSet> desc_sets;
        desc_sets.reserve(sets.size());
        for (const auto& set : sets)
        {
            desc_sets.push_back(set->getHandle().as<VkDescriptorSet>());
            frame_data->resources.insert(set);
        }

        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            static_cast<VkPipelineLayout>(pipeline->getLayoutHandle()),
            0,
            static_cast<uint32_t>(desc_sets.size()),
            desc_sets.data(),
            0,
            nullptr
        );
    }
}

void RenderFrame::draw(const std::shared_ptr<Pipeline>& pipeline, uint32_t vertices, uint32_t instances) const
{
    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    bind(pipeline);

    vkCmdDraw(
        cmdBuffer,
        vertices,
        instances,
        0,
        0);
}

void RenderFrame::draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Buffer>& buffer, uint32_t instances) const
{
    MIDNIGHT_ASSERT(!(buffer->allocated() % pipeline->getBindingStride()), "Buffer stride is not expected by pipeline!");

    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    frame_data->resources.insert(buffer);
    const auto buff = buffer->getHandle().as<VkBuffer>();
    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(
        cmdBuffer,
        0,
        1,
        &buff,
        &off);

    draw(pipeline, buffer->vertices(), instances);
}

void RenderFrame::draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Mesh>& mesh, uint32_t instances) const
{
    if (!mesh->vertexCount()) return;
    
    if (mesh->indexCount())
        drawIndexed(pipeline, mesh->vertex, mesh->index, instances);
    else
        draw(pipeline, mesh->vertex, instances);
}

void RenderFrame::drawIndexed(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Buffer>& buffer, const std::shared_ptr<Buffer>& indices, uint32_t instances) const
{
    MIDNIGHT_ASSERT(!(buffer->allocated() % pipeline->getBindingStride()), "Buffer stride is not expected by pipeline!");

    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    bind(pipeline);

    frame_data->resources.insert(buffer);
    const auto buff = buffer->getHandle().as<VkBuffer>();
    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(
        cmdBuffer,
        0,
        1,
        &buff,
        &off);

    frame_data->resources.insert(indices);
    vkCmdBindIndexBuffer(
        cmdBuffer,
        indices->getHandle().as<VkBuffer>(),
        0,
        VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(
        cmdBuffer,
        indices->allocated() / sizeof(uint32_t),
        instances,
        0, 
        0, 
        0);   
}

}
