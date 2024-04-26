#pragma once

#include <Def.hpp>

namespace mn::Graphics::Backend
{
    struct Fence
    {
        Fence();
        ~Fence();

        void wait() const;
        void reset() const;

        auto getHandle() const { return handle; }

    private:
        Handle<Fence> handle;
    };

    struct Semaphore
    {
        Semaphore();
        ~Semaphore();

        auto getHandle() const { return handle; }

    private:
        Handle<Semaphore> handle;
    };
}