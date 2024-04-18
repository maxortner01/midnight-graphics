#pragma once

#include <Def.hpp>
#include <Utility/Singleton.hpp>

namespace mn::Graphics::Backend
{
    struct Instance : Utility::Singleton<Instance>
    {
        friend class Singleton<Instance>;

        mn::handle_t createSurface(mn::handle_t window) const;
        void destroySurface(mn::handle_t surface) const;

        std::pair<mn::handle_t, std::vector<mn::handle_t>> createSwapchain(mn::handle_t window, mn::handle_t surface) const;
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
        Instance();
        ~Instance();

        void createDevice();

        struct Device
        {
            mn::handle_t handle;
            std::pair<mn::handle_t, uint32_t> graphics_queue;
        } device;

        mn::handle_t handle, physical_device;
    };
}
