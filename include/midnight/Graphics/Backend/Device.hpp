#pragma once

#include <Def.hpp>

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

    private:
        Handle<Device> handle;
        mn::handle_t   physical_device;

        Queue graphics;
    };
}