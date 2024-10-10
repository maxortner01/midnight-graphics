#pragma once

#include <Def.hpp>

namespace mn::Graphics
{
    template<typename T>
    struct ObjectHandle
    {
        ObjectHandle(Handle<T> h = nullptr) : handle(h) { }

        auto getHandle() const { return handle; }

    protected:
        Handle<T> handle;
    };
}