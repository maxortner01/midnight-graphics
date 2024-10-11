#include <Graphics/Backend/Command.hpp>
#include <Graphics/Backend/Sync.hpp>
#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Image.hpp>
#include <Graphics/Buffer.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics::Backend
{

CommandPool::CommandPool()
{
    auto instance = Instance::get();
    handle = instance->getDevice()->createCommandPool();
}

CommandPool::~CommandPool()
{
    if (handle)
    {
        auto instance = Instance::get();
        instance->getDevice()->destroyCommandPool(handle);
        handle = nullptr;
    }
}

void CommandPool::reset() const
{
    auto& device = Instance::get()->getDevice();
    vkResetCommandPool(device->getHandle().as<VkDevice>(), static_cast<VkCommandPool>(handle), VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

std::unique_ptr<CommandBuffer> CommandPool::allocateBuffer() const
{
    return std::make_unique<CommandBuffer>(CommandBuffer(handle));
}

CommandBuffer::CommandBuffer(Handle<CommandPool> pool) :
    handle(Instance::get()->getDevice()->createCommandBuffer(pool))
{   }

void CommandBuffer::begin(bool one_time) const
{
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = ( one_time ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : (VkCommandBufferUsageFlags)0 ),
        .pInheritanceInfo = nullptr,
    };

    const auto err = vkBeginCommandBuffer(handle.as<VkCommandBuffer>(), &begin_info);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error beginning command buffer: " << err);
}

void CommandBuffer::end() const
{
    const auto err = vkEndCommandBuffer(handle.as<VkCommandBuffer>());
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error ending command buffer: " << err);
}

void CommandBuffer::reset() const
{
    const auto err = vkResetCommandBuffer(handle.as<VkCommandBuffer>(), 0);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error reseting command buffer: " << err);
}

void CommandBuffer::submit(std::shared_ptr<Fence> fence) const
{
    auto& device = Instance::get()->getDevice();

    const auto buffer = handle.as<VkCommandBuffer>();
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &buffer;

    const auto err = vkQueueSubmit(
        static_cast<VkQueue>(device->getGraphicsQueue().handle),
        1, 
        &submit, 
        fence->getHandle().as<VkFence>()
    );
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error immediately submitting command buffer");
}

// Should make this a function in Backend::Device, it's copied from RenderFrame.cpp
void __transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
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

void CommandBuffer::bufferToImage(std::shared_ptr<Buffer> buffer, const Image::Attachment& image) const
{
    __transition_image(static_cast<VkCommandBuffer>(handle), static_cast<VkImage>(image.handle), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = { .width = Math::x(image.size), .height = Math::y(image.size), .depth = 1 };

    vkCmdCopyBufferToImage(
        handle.as<VkCommandBuffer>(), 
        buffer->getHandle().as<VkBuffer>(), 
        static_cast<VkImage>(image.handle), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1,
		&copyRegion
    );

    __transition_image(static_cast<VkCommandBuffer>(handle), static_cast<VkImage>(image.handle), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

}