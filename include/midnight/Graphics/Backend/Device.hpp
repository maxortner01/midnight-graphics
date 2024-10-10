#pragma once

#include <Def.hpp>
#include <Math.hpp>

namespace mn::Graphics
{
    struct Window;
    struct Shader;
    struct Image;
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

    struct Sampler
    {
        enum Type
        {
            Nearest, Linear
        };

        mn::handle_t handle;
    };

    struct Device
    {
        Device(Handle<Instance> instance, handle_t p_device);
        ~Device();

        Device(const Device&) = delete;

        Handle<Device> getHandle() const { return handle; }
        mn::handle_t   getPhysicalDevice() const { return physical_device; }
        Queue          getGraphicsQueue() const { return graphics; }

        std::tuple<handle_t, std::vector<handle_t>, uint32_t, Math::Vec2u>  
        createSwapchain(Handle<Window> window, mn::handle_t surface) const;
        
        std::vector<mn::handle_t> getSwapchainImages(mn::handle_t swapchain) const;
        void destroySwapchain(mn::handle_t swapchain) const;

        std::pair<Handle<Image>, mn::handle_t> createImage(const Math::Vec2u& size, uint32_t format, bool depth = false) const;
        void destroyImage(Handle<Image> image, mn::handle_t alloc) const;

        mn::handle_t createImageView(Handle<Image> image, uint32_t format, bool depth = false) const;
        void destroyImageView(mn::handle_t image_view) const;

        Handle<CommandPool> createCommandPool() const;
        void destroyCommandPool(Handle<CommandPool> pool) const;

        Handle<CommandBuffer> createCommandBuffer(Handle<CommandPool> command_pool) const;

        void immediateSubmit(std::function<void(Backend::CommandBuffer&)> func) const;

        Handle<Semaphore> createSemaphore() const;
        void destroySemaphore(Handle<Semaphore> semaphore) const;

        Handle<Fence> createFence(bool signaled = true) const;
        void destroyFence(Handle<Fence> fence) const;

        Handle<Shader> createShader(const std::vector<uint32_t>& data) const;
        void destroyShader(Handle<Shader> shader) const;

        std::shared_ptr<Sampler> getSampler(Sampler::Type type);

        void waitForIdle() const;

        mn::handle_t getImGuiPool();
    private:
        mn::handle_t imgui_pool;

        Handle<Device> handle;
        mn::handle_t   physical_device;

        std::unordered_map<Sampler::Type, std::shared_ptr<Sampler>> samplers;
        Queue graphics;
    };
}