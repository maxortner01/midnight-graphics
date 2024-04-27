#pragma once

#include <Def.hpp>

namespace mn::Graphics
{
    struct Window;
    struct Shader;
}

namespace mn::Graphics::Backend
{
    struct Instance;
    struct CommandPool;
    struct CommandBuffer;
    struct Semaphore;
    struct Fence;

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

        Handle<CommandPool> createCommandPool() const;
        void destroyCommandPool(Handle<CommandPool> pool) const;

        Handle<CommandBuffer> createCommandBuffer(Handle<CommandPool> command_pool) const;

        Handle<Semaphore> createSemaphore() const;
        void destroySemaphore(Handle<Semaphore> semaphore) const;

        Handle<Fence> createFence(bool signaled = true) const;
        void destroyFence(Handle<Fence> fence) const;

        Handle<Shader> createShader(const std::vector<uint32_t>& data) const;
        void destroyShader(Handle<Shader> shader) const;

    private:
        Handle<Device> handle;
        mn::handle_t   physical_device;

        Queue graphics;
    };
}