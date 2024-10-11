#pragma once

#include <Def.hpp>

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Buffer : ObjectHandle<Buffer>
    {
        using gpu_addr = void*;

        MN_SYMBOL Buffer();
        Buffer(Buffer&&);
        Buffer(const Buffer&) = delete;
        virtual ~Buffer() { rawFree(); }

        virtual uint32_t getSize()  const { return 0; }
        virtual uint32_t vertices() const { return 0; }

        void allocateBytes(std::size_t bytes) { rawResize(bytes); }
        auto* rawData() const { return reinterpret_cast<std::byte*>(_data); }
        auto allocated() const { return _size; }

        MN_SYMBOL gpu_addr getAddress() const;

    protected:
        MN_SYMBOL void rawResize(std::size_t newsize);
        MN_SYMBOL void rawFree();
        MN_SYMBOL auto rawSize() const { return _size; }

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

    template<typename T>
    struct Vector 
    {
        Vector() : count{0} { }

        Vector(uint32_t elements)
        {
            resize(elements);
        }

        uint32_t size() const
        {
            return count;
        }

        Buffer::gpu_addr getAddress() const
        {
            return memory.getAddress();
        }

        void reserve(const uint32_t& count)
        {
            // If count < this->count, we need to call the destructors
            for (uint32_t i = count; i < this->count; i++)
                reinterpret_cast<T*>(memory.rawData() + i * sizeof(T))->~T();

            memory.allocateBytes(count * sizeof(T));
        }

        void resize(const uint32_t& count)
        {
            if (count == this->count) return; 
            
            // If count < this->count, we need to call the destructors
            for (uint32_t i = count; i < this->count; i++)
                reinterpret_cast<T*>(memory.rawData() + i * sizeof(T))->~T();

            memory.allocateBytes(count * sizeof(T));

            // default construct each object in the new region
            for (uint32_t i = this->count; i < count; i++)
                new(memory.rawData() + i * sizeof(T)) T();

            this->count = count;
        }

        void push_back(const T& value)
        {
            if (count == memory.allocated() / sizeof(T))
                reserve((memory.allocated() / sizeof(T) + 5) * 1.75f * sizeof(T));
            new(memory.rawData() + count * sizeof(T)) T(value);
        }   

        template<typename... Args>
        void emplace_back(Args&&... args)
        {
            if (count == memory.allocated() / sizeof(T))
                reserve((memory.allocated() / sizeof(T) + 5) * 1.75f * sizeof(T));
            new(memory.rawData() + count * sizeof(T)) T(std::forward<Args>(args)...);
        }

        T& at(uint32_t index)
        {
            return *reinterpret_cast<T*>(memory.rawData() + index * sizeof(T));
        }

        const T& at(uint32_t index) const
        {
            return const_cast<Vector<T>*>(this)->at(index);
        }

        T& operator[](uint32_t index)
        {
            return at(index);
        }

        const T& operator[](uint32_t index) const
        {
            return at(index);
        }

    private:
        uint32_t count;
        Buffer memory;
    };
}