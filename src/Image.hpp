#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <filesystem>
#include <optional>

class AllocatedImage
{
  public:
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;

    std::optional<VkSampler> imageSampler;

  public:
    void create(VkDevice device, VmaAllocator allocator, VkExtent3D extent, VkFormat format,
                VkImageUsageFlags usage);

    void load(VkDevice device, VmaAllocator allocator, std::filesystem::path file,
              VkImageUsageFlags usage);

    void createSampler(VkDevice device, VkFilter filter);

    void destroy(VkDevice device, VmaAllocator allocator);

    static void transition(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
                           VkImageLayout newLayout);

    static void copyImgToImg(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize,
                             VkExtent2D dstSize);
};
