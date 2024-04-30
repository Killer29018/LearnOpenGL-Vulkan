#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <memory>

#include "Buffer.hpp"
#include "Camera.hpp"
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

struct ObjectData {
    alignas(16) int materialIndex;
    alignas(16) glm::vec4 colour;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 rotation;
};

struct LightGeneralData {
    alignas(16) int lightCount;
    alignas(16) glm::vec4 ambient;
};

struct LightData {
    alignas(16) glm::vec3 position;
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
    alignas(16) glm::vec3 attenuation;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 view[6];
    alignas(16) glm::vec2 planes;
};

struct MaterialData {
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec4 specular;
};

struct Vertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec2 uv;
    alignas(16) glm::vec3 normal;
};

struct VertexPushConstant {
    alignas(8) glm::mat4 view;
    alignas(8) glm::mat4 proj;
    alignas(8) glm::vec3 cameraPos;
    alignas(8) VkDeviceAddress vertexBuffer;
};

struct ShadowPushConstant {
    alignas(8) VkDeviceAddress vertexBuffer;
    alignas(8) glm::ivec2 currentLight;
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

    void createMaterials();
    void createObjects();

    void createLights();
    void uploadLightData();

    void initPipelines();

    void initDescriptorPool();
    void initDescriptorSets();

    void createMesh();

    FrameData& getCurrentFrame();

    void renderShadow(VkCommandBuffer& cmd);
    void renderGeometry(VkCommandBuffer& cmd);

    void update();
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

    VkDescriptorSetLayout m_ObjectDescriptorLayout;
    std::vector<VkDescriptorSet> m_ObjectDescriptors;

    VkDescriptorSetLayout m_LightDescriptorLayout;
    std::vector<VkDescriptorSet> m_LightDescriptors;

    VkDescriptorSetLayout m_MaterialDescriptorLayout;
    std::vector<VkDescriptorSet> m_MaterialDescriptors;

    size_t m_ObjectCount;
    std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT> m_ObjectDataBuffer;

    static constexpr size_t m_MaxLights = 10;
    size_t m_LightCount = 0;
    float m_LightTime = 0.0f;
    std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT> m_LightDataBuffer;
    AllocatedImage m_ShadowMaps;

    glm::mat4 cameraView, cameraProj;

    static constexpr size_t m_MaxMaterials = 10;
    std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT> m_MaterialDataBuffer;

    VkPipelineLayout m_MeshPipelineLayout;
    VkPipeline m_MeshPipeline;

    VkPipelineLayout m_LightPipelineLayout;
    VkPipeline m_LightPipeline;

    VkPipelineLayout m_ShadowMapPipelineLayout;
    VkPipeline m_ShadowMapPipeline;

    Mesh m_BasicMesh;

    Camera m_Camera;

    size_t m_CurrentFrame = 0;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
};
