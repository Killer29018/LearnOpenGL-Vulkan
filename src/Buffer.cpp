#include "Buffer.hpp"

#include "ErrorCheck.hpp"

void AllocatedBuffer::createBuffer(VmaAllocator allocator, size_t allocSize,
                                   VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.pNext = nullptr;
    bufferCI.size = allocSize;
    bufferCI.usage = usage;

    VmaAllocationCreateInfo vmaAllocCI{};
    vmaAllocCI.usage = memoryUsage;
    vmaAllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VK_CHECK(
        vmaCreateBuffer(allocator, &bufferCI, &vmaAllocCI, &buffer, &allocation, &allocationInfo));
}

void AllocatedBuffer::destroyBuffer(VmaAllocator allocator)
{
    vmaDestroyBuffer(allocator, buffer, allocation);
    buffer = VK_NULL_HANDLE;
    allocation = {};
}
