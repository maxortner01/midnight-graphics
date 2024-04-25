#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Sync.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics::Backend
{
Fence::Fence()
{
    auto& device = Instance::get()->getDevice();
    handle = device->createFence();
}

Fence::~Fence()
{
    if (handle)
    {
        auto& device = Instance::get()->getDevice();
        device->destroyFence(handle);
        handle = nullptr;
    }
}

void Fence::wait() const
{
    auto& device = Instance::get()->getDevice();
    auto fence = handle.as<VkFence>();
    const auto err = vkWaitForFences(device->getHandle().as<VkDevice>(), 1, &fence, true, 1000000000);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error waiting on fence: " << err);
}

void Fence::reset() const
{
    auto& device = Instance::get()->getDevice();
    auto fence = handle.as<VkFence>();
    const auto err = vkResetFences(device->getHandle().as<VkDevice>(), 1, &fence);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error reseting fence: " << err);
}

Semaphore::Semaphore()
{
    auto& device = Instance::get()->getDevice();
    handle = device->createSemaphore();
}

Semaphore::~Semaphore()
{
    if (handle)
    {
        auto& device = Instance::get()->getDevice();
        device->destroySemaphore(handle);
        handle = nullptr;
    }
}
}