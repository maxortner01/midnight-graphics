#include <Graphics/Buffer.hpp>

#include <Graphics/Backend/Instance.hpp>

// Hack to get rid of annoying warnings from VMA
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma clang diagnostic pop

namespace mn::Graphics
{
    Buffer::Buffer() :
        _data(nullptr), _size{0}
    {   }

    void Buffer::rawFree()
    {
        const auto allocator = static_cast<VmaAllocator>(Backend::Instance::get()->getAllocator());
        vmaDestroyBuffer(allocator, handle.as<VkBuffer>(), allocation.as<VmaAllocation>());
        handle = nullptr;
        allocation = nullptr;
    }

    void Buffer::rawResize(std::size_t newsize)
    {
        if (!newsize)
        {
            if (handle) rawFree();
            return;
        }

        const auto old_size = _size;
        void* current_data = nullptr;
        if (handle) 
        {
            current_data = std::malloc(_size);
            std::memcpy(current_data, _data, _size);
            rawFree();
        }

        VkBufferCreateInfo buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = newsize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        };

        VmaAllocationCreateInfo alloc_create_info = {
            .usage = VMA_MEMORY_USAGE_AUTO,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
        };

        const auto allocator = static_cast<VmaAllocator>(Backend::Instance::get()->getAllocator());
        VkBuffer buff;
        VmaAllocation alloc;
        VmaAllocationInfo info;
        vmaCreateBuffer(allocator, &buffer_create_info, &alloc_create_info, &buff, &alloc, &info);
        
        allocation = alloc;
        handle = buff;

        _data = info.pMappedData;
        _size = info.size;

        if (current_data) 
        {
            std::memcpy(_data, current_data, old_size);
            std::free(current_data);
        }
    }
}