#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <span>

#include "ImmediateSubmit.hpp"

class AllocatedBuffer
{
  public:
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

  public:
    void createBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage,
                      VmaMemoryUsage memoryUsage);

    void destroyBuffer(VmaAllocator allocator);

    void pushFromBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer, size_t size);

    template<typename T>
    void pushData(VmaAllocator allocator, const std::span<T>& data)
    {
        size_t size = data.size() * sizeof(T);
        AllocatedBuffer stagingBuffer;
        stagingBuffer.createBuffer(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(stagingBuffer.allocationInfo.pMappedData, data.data(), size);

        ImmediateSubmit::submit([&](VkCommandBuffer cmd) {
            VkBufferCopy copy{};
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = size;

            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer, 1, &copy);
        });

        stagingBuffer.destroyBuffer(allocator);
    }

  private:
    bool m_Init = false;
};
