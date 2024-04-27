#include <Graphics/Window.hpp>
#include <Graphics/RenderFrame.hpp>
#include <Graphics/Pipeline.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics
{

void RenderFrame::startRender()
{
    VkRenderingAttachmentInfo color_attach = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .
    };

    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .color
    }
    //vkCmdBeginRendering()
}

void RenderFrame::endRender()
{

}

void RenderFrame::clear(std::tuple<float, float, float> color) const
{   
    VkClearColorValue clearValue;
	clearValue = { { std::get<0>(color), std::get<1>(color), std::get<2>(color), 1.0f } };

	VkImageSubresourceRange clearRange = [](VkImageAspectFlags aspectMask)
    {
        return VkImageSubresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS
        };
    }(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	vkCmdClearColorImage(
        frame_data->command_buffer->getHandle().as<VkCommandBuffer>(), 
        static_cast<VkImage>(image), 
        VK_IMAGE_LAYOUT_GENERAL, 
        &clearValue, 
        1, 
        &clearRange);
}

void RenderFrame::draw(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer) const
{
    MIDNIGHT_ASSERT(buffer->getSize() == pipeline.getBindingStride(), "Buffer stride is not expected by pipeline");

    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    vkCmdBindPipeline(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.getHandle().as<VkPipeline>());

    const auto buff = buffer->getHandle().as<VkBuffer>();
    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(
        cmdBuffer,
        0,
        1,
        &buff,
        &off);

    vkCmdDraw(
        cmdBuffer,
        buffer->vertices(),
        1, 
        0,
        0);
}

}