#pragma once

#include <Def.hpp>
#include <Utility/Singleton.hpp>

#include "Device.hpp"

namespace mn::Graphics
{
    struct Window;
}

namespace mn::Graphics::Backend
{
    enum class Untyped
    {
        Surface
    };

    struct Instance : Utility::Singleton<Instance>
    {
        friend class Singleton<Instance>;

        mn::handle_t createSurface(Handle<Window> window) const;
        void destroySurface(mn::handle_t surface) const;

        auto getAllocator()  const { return allocator; }

        const std::unique_ptr<Device>& getDevice() const
        {
            return device;
        }

    private:
        Instance();
        ~Instance();

        Handle<Instance> handle;
        mn::handle_t allocator;

        std::unique_ptr<Device> device;
    };
}
