#pragma once

#include <Def.hpp>

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Buffer : ObjectHandle<Buffer>
    {
        Buffer();
        Buffer(Buffer&&) = default;
        Buffer(const Buffer&) = delete;
        virtual ~Buffer() { rawFree(); }

        virtual uint32_t getSize() const = 0;
        virtual uint32_t vertices() const { return 0; }

    protected:
        void  rawResize(std::size_t newsize);
        void  rawFree();
        auto  rawSize() const { return _size; }
        auto* rawData() const { return reinterpret_cast<std::byte*>(_data); }

    private:
        Handle<Buffer> allocation;
        void* _data;
        std::size_t _size;
    };

    template<typename T>
    struct TypeBuffer : Buffer
    {
        uint32_t getSize() const override { return sizeof(T); }
        uint32_t vertices() const override { return size(); }

        std::size_t size() const
        {
            MIDNIGHT_ASSERT(rawSize() % sizeof(T) == 0, "Buffer incorrectly allocated");
            return rawSize() / sizeof(T);
        }

        T& at(std::size_t index)
        {
            MIDNIGHT_ASSERT(index < size(), "Index out of bounds");
            return *(reinterpret_cast<T*>(rawData()) + index);
        }

        const T& at(std::size_t index) const
        {
            return const_cast<TypeBuffer<T>*>(this)->at(index);
        }

        T& operator[](std::size_t index)
        {
            return at(index);
        }

        const T& operator[](std::size_t index) const
        {
            return at(index);
        }

        void resize(std::size_t count)
        {
            this->rawResize(count * sizeof(T));
        }
    };
}