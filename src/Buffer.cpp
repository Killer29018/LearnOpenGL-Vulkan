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

    m_Init = true;
}

void AllocatedBuffer::destroyBuffer(VmaAllocator allocator)
{
    if (!m_Init) return;

    vmaDestroyBuffer(allocator, buffer, allocation);
    buffer = VK_NULL_HANDLE;
    allocation = {};

    m_Init = false;
}

void AllocatedBuffer::pushFromBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer,
                                     size_t size)
{
    ImmediateSubmit::submit([&](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = size;

        vkCmdCopyBuffer(cmd, buffer.buffer, this->buffer, 1, &copy);
    });
}
