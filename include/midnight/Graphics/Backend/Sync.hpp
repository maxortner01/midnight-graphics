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

    private:
        Handle<Fence> handle;
    };

    struct Semaphore
    {
        Semaphore();
        ~Semaphore();

    private:
        Handle<Semaphore> handle;
    };
}