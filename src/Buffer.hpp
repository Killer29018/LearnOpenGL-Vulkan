#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

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
};
