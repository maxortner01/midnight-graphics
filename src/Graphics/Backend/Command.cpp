#include <Graphics/Backend/Command.hpp>

#include <Graphics/Backend/Instance.hpp>

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

}