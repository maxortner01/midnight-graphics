#pragma once

#include <Def.hpp>

namespace mn::Graphics
{
    template<typename T>
    struct ObjectHandle
    {
        ObjectHandle() = default;
        ObjectHandle(Handle<T> h) : handle(h) { }

        auto getHandle() const { return handle; }

    protected:
        Handle<T> handle;
    };
}