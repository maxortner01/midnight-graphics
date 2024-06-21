#include <Graphics/Backend/Command.hpp>
#include <Graphics/Backend/Instance.hpp>

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

}