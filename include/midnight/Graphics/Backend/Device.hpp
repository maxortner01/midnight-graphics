#pragma once

#include <Def.hpp>

namespace mn::Graphics
{
    struct Window;
}

namespace mn::Graphics::Backend
{
    struct Instance;

    struct Queue
    {
        mn::handle_t handle;
        uint32_t index;
    };

    struct Device
    {
        Device(Handle<Instance> instance, handle_t p_device);
        ~Device();

        Device(const Device&) = delete;

        Handle<Device> getHandle() const { return handle; }
        mn::handle_t   getPhysicalDevice() const { return physical_device; }
        Queue          getGraphicsQueue() const { return graphics; }

        std::pair<mn::handle_t, std::vector<mn::handle_t>> createSwapchain(Handle<Window> window, mn::handle_t surface) const;
        std::vector<mn::handle_t> getSwapchainImages(mn::handle_t swapchain) const;
        void destroySwapchain(mn::handle_t swapchain) const;

        void destroyImageView(mn::handle_t image_view) const;

        mn::handle_t createCommandPool() const;
        void destroyCommandPool(mn::handle_t pool) const;

        mn::handle_t createCommandBuffer(mn::handle_t command_pool) const;

        mn::handle_t createSemaphore() const;
        void destroySemaphore(mn::handle_t semaphore) const;

        mn::handle_t createFence(bool signaled = true) const;
        void destroyFence(mn::handle_t fence) const;

    private:
        Handle<Device> handle;
        mn::handle_t   physical_device;

        Queue graphics;
    };
}