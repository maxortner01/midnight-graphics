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

        void begin(bool one_time = true) const;
        void end() const;
        void reset() const;

        auto getHandle() const { return handle; }

    private:    
        CommandBuffer(Handle<CommandPool> pool);

        Handle<CommandBuffer> handle;
    };
}