#pragma once

#include <Def.hpp>

namespace mn::Graphics
{
    struct Image;
    struct Buffer;
}

namespace mn::Graphics::Backend
{
    struct CommandBuffer;
    struct Fence;

    struct CommandPool
    {
        CommandPool();
        ~CommandPool();

        CommandPool(const CommandPool&) = delete;
        CommandPool(CommandPool&&) = default;

        void reset() const;
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
        void submit(std::shared_ptr<Fence> fence) const;
        void bufferToImage(std::shared_ptr<Buffer> buffer, std::shared_ptr<Image> image) const;

    private:    
        CommandBuffer(Handle<CommandPool> pool);

        Handle<CommandBuffer> handle;
    };
}