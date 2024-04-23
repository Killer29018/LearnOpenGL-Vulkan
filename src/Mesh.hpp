#pragma once

#include <cstring>
#include <span>

#include <vk_mem_alloc.h>

#include "Buffer.hpp"
#include "ImmediateSubmit.hpp"

struct VmaAllocation_T;

class Mesh
{
  public:
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;

  public:
    template<typename T>
    void createMesh(VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                    std::span<T> vertices)
    {
        const size_t vertexBufferSize = vertices.size() * sizeof(T);
        const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        vertexBuffer.createBuffer(allocator, vertexBufferSize,
                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                  VMA_MEMORY_USAGE_GPU_ONLY);
        indexBuffer.createBuffer(allocator, indexBufferSize,
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VMA_MEMORY_USAGE_GPU_ONLY);

        VkBufferDeviceAddressInfo deviceAI{};
        deviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAI.pNext = nullptr;
        deviceAI.buffer = vertexBuffer.buffer;

        vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAI);

        AllocatedBuffer staging;
        staging.createBuffer(allocator, vertexBufferSize + indexBufferSize,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_COPY);

        void* data;
        vmaMapMemory(allocator, staging.allocation, &data);
        memcpy(data, vertices.data(), vertexBufferSize);
        memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);
        vmaUnmapMemory(allocator, staging.allocation);

        ImmediateSubmit::submit([&](VkCommandBuffer cmd) {
            VkBufferCopy vertexCopy{};
            vertexCopy.srcOffset = 0;
            vertexCopy.dstOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy{};
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.dstOffset = 0;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, indexBuffer.buffer, 1, &indexCopy);
        });

        staging.destroyBuffer(allocator);
    }

    void destroyMesh(VmaAllocator allocator)
    {
        vertexBuffer.destroyBuffer(allocator);
        indexBuffer.destroyBuffer(allocator);
        vertexBufferAddress = 0;
    }
};
