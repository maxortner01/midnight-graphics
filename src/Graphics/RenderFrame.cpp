#include <Graphics/Window.hpp>
#include <Graphics/RenderFrame.hpp>
#include <Graphics/Pipeline.hpp>

#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Device.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics
{

void RenderFrame::startRender()
{
    VkRenderingAttachmentInfo color_attach = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = image->getColorImageView().as<VkImageView>(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attach,
        .renderArea = { 0, 0, image->size().first, image->size().second }
    };

    const auto command_buffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    VkRect2D sc = { 0, 0, image->size().first, image->size().second };
    vkCmdSetScissor(command_buffer, 0, 1, &sc);

    VkViewport extent = { .x = 0, .y = 0, .width = static_cast<float>(image->size().first), .height = static_cast<float>(image->size().second), .minDepth = 0, .maxDepth = 1.f };
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
        image->getHandle().as<VkImage>(), 
        VK_IMAGE_LAYOUT_GENERAL, 
        &clearValue, 
        1, 
        &clearRange);
}

void RenderFrame::draw(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer, uint32_t desc_index) const
{
    MIDNIGHT_ASSERT(buffer->getSize() == pipeline.getBindingStride(), "Buffer stride is not expected by pipeline");

    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    vkCmdBindPipeline(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.getHandle().as<VkPipeline>());

    pipeline.bindDescriptorSet(desc_index, frame_data->command_buffer);

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
