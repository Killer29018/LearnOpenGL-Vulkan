#include "Image.hpp"

#include "Buffer.hpp"
#include "ErrorCheck.hpp"
#include "ImmediateSubmit.hpp"

#include <stb_image.h>

#include <cstring>
#include <format>
#include <iostream>

void AllocatedImage::create(VkDevice device, VmaAllocator allocator, VkExtent3D extent,
                            VkFormat format, VkImageUsageFlags usage)
{
    imageFormat = format;
    imageExtent = extent;

    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.pNext = nullptr;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent = extent;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = usage;

    VmaAllocationCreateInfo allocateCI{};
    allocateCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocateCI.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vmaCreateImage(allocator, &imageCI, &allocateCI, &image, &allocation, nullptr));

    VkImageAspectFlags aspectFlags =
        (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.pNext = nullptr;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = image;
    imageViewCI.format = format;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = imageCI.mipLevels;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = aspectFlags;

    VK_CHECK(vkCreateImageView(device, &imageViewCI, nullptr, &imageView));
}

void AllocatedImage::load(VkDevice device, VmaAllocator allocator, std::filesystem::path file,
                          VkImageUsageFlags usage)
{
    int width, height, channels;
    uint8_t* data = stbi_load(file.c_str(), &width, &height, &channels, 4);

    if (data)
    {
        imageExtent = VkExtent3D{ (uint32_t)width, (uint32_t)height, 1 };

        size_t size = imageExtent.width * imageExtent.height * imageExtent.depth * 4;

        AllocatedBuffer uploadBuffer;
        uploadBuffer.createBuffer(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(uploadBuffer.allocationInfo.pMappedData, data, size);

        create(device, allocator, imageExtent, VK_FORMAT_R8G8B8A8_UNORM,
               usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        ImmediateSubmit::submit([&](VkCommandBuffer cmd) {
            AllocatedImage::transition(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion{};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;

            vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            AllocatedImage::transition(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        uploadBuffer.destroyBuffer(allocator);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << std::format("Failed to load Image: {}\n", file.c_str());
    }
}

void AllocatedImage::createSampler(VkDevice device, VkFilter filter)
{
    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.pNext = nullptr;
    samplerCI.flags = 0;
    samplerCI.minFilter = filter;
    samplerCI.magFilter = filter;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCI.maxLod = VK_LOD_CLAMP_NONE;
    samplerCI.minLod = 0;

    VkSampler sampler;
    VK_CHECK(vkCreateSampler(device, &samplerCI, nullptr, &sampler));

    imageSampler = sampler;
}

void AllocatedImage::destroy(VkDevice device, VmaAllocator allocator)
{
    if (imageSampler.has_value()) vkDestroySampler(device, imageSampler.value(), nullptr);

    vkDestroyImageView(device, imageView, nullptr);
    vmaDestroyImage(allocator, image, allocation);
}

void AllocatedImage::transition(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
                                VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                        ? VK_IMAGE_ASPECT_DEPTH_BIT
                                        : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.pNext = nullptr;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.subresourceRange.aspectMask = aspectMask;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    imageBarrier.image = image;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pNext = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

void AllocatedImage::copyImgToImg(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize,
                                  VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.pNext = nullptr;
    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.pNext = nullptr;
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImage = dst;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}
