#pragma once

#include <Def.hpp>

namespace mn::Graphics::Backend
{
    struct CommandBuffer;

    struct CommandPool
    {
        CommandPool();
        ~CommandPool();

        CommandPool(const CommandPool&) = delete;
        CommandPool(CommandPool&&) = default;

        std::unique_ptr<CommandBuffer> allocateBuffer() const;

    private:
        Handle<CommandPool> handle;
    };

    struct CommandBuffer
    {
        friend struct CommandPool;

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer(CommandBuffer&&) = default;

    private:    
        CommandBuffer(Handle<CommandPool> pool);

        Handle<CommandBuffer> handle;
    };
}