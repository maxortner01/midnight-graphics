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
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    std::optional<VkRenderingAttachmentInfo> depth_attach;

    if (image->getDepthImageView())
    {
        auto attachment_info = VkRenderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = image->getDepthImageView().as<VkImageView>(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .depthStencil = { .depth = 1 } }
        };

        depth_attach.emplace(attachment_info);
    }

    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { 0, 0, Math::x( image->size() ), Math::y( image->size() ) },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attach,
        .pDepthAttachment = ( depth_attach.has_value() ? &(*depth_attach) : nullptr )
    };

    const auto command_buffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    VkRect2D sc = { 0, 0, Math::x( image->size() ), Math::y( image->size() ) };
    vkCmdSetScissor(command_buffer, 0, 1, &sc);

    VkViewport extent = { .x = 0, .y = 0, .width = static_cast<float>(Math::x(image->size())), .height = static_cast<float>(Math::y(image->size())), .minDepth = 0, .maxDepth = 1.f };
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

	//clear image
	vkCmdClearColorImage(
        frame_data->command_buffer->getHandle().as<VkCommandBuffer>(), 
        image->getHandle().as<VkImage>(), 
        VK_IMAGE_LAYOUT_GENERAL, 
        &clearValue, 
        1, 
        &colorRange);

    /*
    vkCmdClearDepthStencilImage(
        frame_data->command_buffer->getHandle().as<VkCommandBuffer>(), 
        image->getDepthImage().as<VkImage>(), 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
        &depthvalue, 
        1, 
        &dsRange);*/
}

void RenderFrame::setPushConstant(const Pipeline& pipeline, const void* data) const
{
    pipeline.setPushConstant(frame_data->command_buffer, data);
}

void RenderFrame::draw(const Pipeline& pipeline, uint32_t vertices) const
{
    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    vkCmdBindPipeline(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.getHandle().as<VkPipeline>());

    vkCmdDraw(
        cmdBuffer,
        vertices,
        1, 
        0,
        0);
}

void RenderFrame::draw(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer) const
{
    MIDNIGHT_ASSERT(!(buffer->allocated() % pipeline.getBindingStride()), "Buffer stride is not expected by pipeline!");

    const auto cmdBuffer = frame_data->command_buffer->getHandle().as<VkCommandBuffer>();

    const auto buff = buffer->getHandle().as<VkBuffer>();
    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(
        cmdBuffer,
        0,
        1,
        &buff,
        &off);

    draw(pipeline, buffer->vertices());
}

void RenderFrame::draw(const Pipeline& pipeline, const Model& model) const
{
    if (!model.vertexCount()) return;

    if (model.indexCount())
        drawIndexed(pipeline, model.vertex, model.index);
    else
        draw(pipeline, model.vertex);
}

void RenderFrame::draw(const Pipeline& pipeline, std::shared_ptr<Model> model) const
{
    draw(pipeline, *model);
}

void RenderFrame::drawIndexed(const Pipeline& pipeline, std::shared_ptr<Buffer> buffer, std::shared_ptr<Buffer> indices) const
{
    MIDNIGHT_ASSERT(!(buffer->allocated() % pipeline.getBindingStride()), "Buffer stride is not expected by pipeline!");

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

    vkCmdBindIndexBuffer(
        cmdBuffer,
        indices->getHandle().as<VkBuffer>(),
        0,
        VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(
        cmdBuffer,
        indices->allocated() / sizeof(uint32_t),
        1,
        0, 
        0, 
        0);
}

}
