#pragma once

#include "vulkan/vk_enum_string_helper.h"
#include "vulkan/vulkan.h"

#include <format>

#if 1
#define VK_CHECK(x)                                                                                \
    do                                                                                             \
    {                                                                                              \
        VkResult result = x;                                                                       \
        if (result)                                                                                \
        {                                                                                          \
            throw std::runtime_error(                                                              \
                std::format("Error| {}:{} | {}", __FILE__, __LINE__, string_VkResult(result)));    \
        }                                                                                          \
    } while (0)
#else
inline void VK_CHECK(VkResult x)
{
    VkResult result = x;
    if (result)
    {
        std::runtime_error(
            std::format("Error| {}:{} | {}", __FILE__, __LINE__, string_VkResult(result)));
    }
}
#endif
