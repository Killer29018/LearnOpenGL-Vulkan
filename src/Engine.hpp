#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <memory>

#include "Buffer.hpp"
#include "Descriptors.hpp"
#include "EventHandler.hpp"
#include "Image.hpp"
#include "ImmediateSubmit.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkFence renderFence;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
};

struct Vertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec2 uv;
};

struct VertexPushConstant {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) VkDeviceAddress vertexBuffer;
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

    void initDescriptorSetLayouts();

    void initTextures();

    void initPipelines();

    void initDescriptorPool();
    void initDescriptorSets();

    void createMesh();

    FrameData& getCurrentFrame();

    void renderGeometry(VkCommandBuffer& cmd);

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

    AllocatedImage m_DrawImage;
    AllocatedImage m_DepthImage;

    AllocatedImage m_BoxTexture;
    AllocatedImage m_FaceTexture;

    VkDescriptorPool m_DescriptorPool;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_MainDescriptors;
    VkDescriptorSetLayout m_MainDescriptorLayout;

    Pipeline m_BasicPipeline;

    Mesh m_BasicMesh;

    size_t m_CurrentFrame = 0;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
};
