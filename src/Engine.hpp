#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <memory>

#include "EventHandler.hpp"
#include "Image.hpp"
#include "Window.hpp"

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkFence renderFence;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
};

class Engine : public EventObserver
{
  public:
    Engine();
    virtual ~Engine();

    void receiveEvent(const Event* event) override;

  private:
    void cleanup();

    void initVulkan();

    void destroySwapchain();
    void createSwapchain();
    void initSwapchain();

    void initCommands();
    void initSyncStructures();

    FrameData& getCurrentFrame();
    void render();
    void mainLoop();

  private:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    std::unique_ptr<Window> m_Window;

    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    VkSurfaceKHR m_Surface;
    VkQueue m_GraphicsQueue;
    uint32_t m_GraphicsQueueFamily;
    VmaAllocator m_Allocator;

    VkSwapchainKHR m_Swapchain;
    VkFormat m_SwapchainImageFormat;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkExtent2D m_SwapchainImageExtent;

    ImageAllocation m_DrawImage;

    size_t m_CurrentFrame = 0;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
};
