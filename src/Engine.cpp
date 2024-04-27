#include "Engine.hpp"

#include <iostream>

#include <VkBootstrap.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ErrorCheck.hpp"
#include "Image.hpp"
#include "Pipeline.hpp"

Engine::Engine()
{
    m_Window = std::make_unique<Window>();
    m_Window->init("LearnOpenGL-Vulkan", { 800, 800 });

    m_Camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f), 0.0f, 0.0f);

    m_Window->attachObserver(this);
    m_Window->attachObserver(&m_Camera);

    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructures();

    ImmediateSubmit::init(m_Device, m_GraphicsQueue, m_GraphicsQueueFamily);

    initDescriptorSetLayouts();

    initPipelines();

    initTextures();

    createMaterials();
    createObjects();
    createLights();

    initDescriptorPool();
    initDescriptorSets();

    createMesh();

    mainLoop();
}

Engine::~Engine()
{
    vkDeviceWaitIdle(m_Device);
    cleanup();
}

void Engine::receiveEvent(const Event* event)
{
    switch (event->getType())
    {
    case EventType::KEYBOARD_PRESS:
        {
            break;
        }
    default:
        break;
    }
}

void Engine::cleanup()
{
    m_BasicMesh.destroyMesh(m_Allocator);

    ImmediateSubmit::free();

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_MaterialDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_ObjectDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_LightDescriptorLayout, nullptr);

    for (size_t i = 0; i < m_ObjectDataBuffer.size(); i++)
    {
        m_LightDataBuffer[i].destroyBuffer(m_Allocator);
        m_ObjectDataBuffer[i].destroyBuffer(m_Allocator);
        m_MaterialDataBuffer[i].destroyBuffer(m_Allocator);
    }

    m_FaceTexture.destroy(m_Device, m_Allocator);
    m_BoxTexture.destroy(m_Device, m_Allocator);

    vkDestroyPipeline(m_Device, m_LightPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_LightPipelineLayout, nullptr);

    vkDestroyPipeline(m_Device, m_MeshPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_MeshPipelineLayout, nullptr);
    for (size_t i = 0; i < m_Frames.size(); i++)
    {
        vkDestroySemaphore(m_Device, m_Frames[i].renderSemaphore, nullptr);
        vkDestroySemaphore(m_Device, m_Frames[i].swapchainSemaphore, nullptr);
        vkDestroyFence(m_Device, m_Frames[i].renderFence, nullptr);

        vkDestroyCommandPool(m_Device, m_Frames[i].commandPool, nullptr);
    }

    m_DrawImage.destroy(m_Device, m_Allocator);
    m_DepthImage.destroy(m_Device, m_Allocator);
    destroySwapchain();

    vmaDestroyAllocator(m_Allocator);

    vkDestroyDevice(m_Device, nullptr);

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

    vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    vkDestroyInstance(m_Instance, nullptr);
}

void Engine::initVulkan()
{
    vkb::InstanceBuilder builder;
    auto instRet = builder.set_app_name("LearnOpenGL-Vulkan")
                       .request_validation_layers(true)
                       .use_default_debug_messenger()
                       .require_api_version(1, 3, 0)
                       .build();

    vkb::Instance vkbInst = instRet.value();
    m_Instance = vkbInst.instance;
    m_DebugMessenger = vkbInst.debug_messenger;

    m_Surface = m_Window->getSurface(m_Instance);

    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    VkPhysicalDeviceVulkan11Features features11{};
    features11.shaderDrawParameters = true;

    VkPhysicalDeviceFeatures features{};
    features.robustBufferAccess = true;

    vkb::PhysicalDeviceSelector selector{ vkbInst };
    auto vkbMaybeDevice = selector.set_minimum_version(1, 3)
                              .set_required_features_13(features13)
                              .set_required_features_12(features12)
                              .set_required_features_11(features11)
                              .set_required_features(features)
                              .set_surface(m_Surface)
                              .select();

    if (!vkbMaybeDevice.has_value())
    {
        std::cout << std::format("{}: {}", vkbMaybeDevice.error().value(),
                                 vkbMaybeDevice.error().message());
    }

    vkb::PhysicalDevice vkbPhysicalDevice = vkbMaybeDevice.value();

    vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    m_PhysicalDevice = vkbPhysicalDevice.physical_device;
    m_Device = vkbDevice.device;

    m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorCI{};
    allocatorCI.physicalDevice = m_PhysicalDevice;
    allocatorCI.device = m_Device;
    allocatorCI.instance = m_Instance;
    allocatorCI.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorCI, &m_Allocator);
}

void Engine::destroySwapchain()
{
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
    {
        vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
    }

    m_SwapchainImages.clear();
    m_SwapchainImageViews.clear();
}

void Engine::createSwapchain()
{
    vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

    m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain =
        swapchainBuilder
            .set_desired_format({ .format = m_SwapchainImageFormat,
                                  .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(m_Window->getSize().x, m_Window->getSize().y)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    m_Swapchain = vkbSwapchain.swapchain;
    m_SwapchainImageExtent = vkbSwapchain.extent;
    m_SwapchainImages = vkbSwapchain.get_images().value();
    m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
}

void Engine::initSwapchain()
{
    createSwapchain();

    m_DrawImage.create(m_Device, m_Allocator,
                       { (uint32_t)m_Window->getSize().x, (uint32_t)m_Window->getSize().y, 1 },
                       VK_FORMAT_R16G16B16A16_UNORM,
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    m_DepthImage.create(m_Device, m_Allocator,
                        { (uint32_t)m_Window->getSize().x, (uint32_t)m_Window->getSize().y, 1 },
                        VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Engine::initCommands()
{
    VkCommandPoolCreateInfo commandPoolCI{};
    commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCI.pNext = nullptr;
    commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCI.queueFamilyIndex = m_GraphicsQueueFamily;

    for (size_t i = 0; i < m_Frames.size(); i++)
    {
        VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolCI, nullptr, &m_Frames[i].commandPool));

        VkCommandBufferAllocateInfo commandBufferAI{};
        commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAI.pNext = nullptr;
        commandBufferAI.commandPool = m_Frames[i].commandPool;
        commandBufferAI.commandBufferCount = 1;
        commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VK_CHECK(
            vkAllocateCommandBuffers(m_Device, &commandBufferAI, &m_Frames[i].mainCommandBuffer));
    }
}

void Engine::initSyncStructures()
{
    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.pNext = nullptr;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCI.pNext = nullptr;

    for (size_t i = 0; i < m_Frames.size(); i++)
    {
        VK_CHECK(vkCreateFence(m_Device, &fenceCI, nullptr, &m_Frames[i].renderFence));

        VK_CHECK(
            vkCreateSemaphore(m_Device, &semaphoreCI, nullptr, &m_Frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCI, nullptr, &m_Frames[i].renderSemaphore));
    }
}

void Engine::initDescriptorSetLayouts()
{
    m_ObjectDescriptorLayout = DescriptorLayoutBuilder::start(m_Device)
                                   .addCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT)
                                   .addCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT)
                                   .addStorageBuffer(2, VK_SHADER_STAGE_VERTEX_BIT)
                                   .build();

    m_LightDescriptorLayout =
        DescriptorLayoutBuilder::start(m_Device)
            .addStorageBuffer(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    m_MaterialDescriptorLayout = DescriptorLayoutBuilder::start(m_Device)
                                     .addStorageBuffer(0, VK_SHADER_STAGE_FRAGMENT_BIT)
                                     .build();
}

void Engine::initPipelines()
{
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(VertexPushConstant);

    {
        m_MeshPipelineLayout = PipelineLayoutBuilder::build(
            m_Device, { pushConstant },
            { m_LightDescriptorLayout, m_ObjectDescriptorLayout, m_MaterialDescriptorLayout });

        std::optional<VkShaderModule> vertShaderModule =
            PipelineBuilder::createShaderModule(m_Device, "res/shaders/mesh.vert.spv");
        std::optional<VkShaderModule> fragShaderModule =
            PipelineBuilder::createShaderModule(m_Device, "res/shaders/mesh.frag.spv");

        m_MeshPipeline =
            PipelineBuilder::start(m_Device, m_MeshPipelineLayout)
                .setShaders(vertShaderModule.value(), fragShaderModule.value())
                .inputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .rasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                .setMultisampleNone()
                .disableBlending()
                .setColourAttachmentFormat(m_DrawImage.imageFormat)
                .setDepthFormat(m_DepthImage.imageFormat)
                .enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS)
                .build();

        vkDestroyShaderModule(m_Device, vertShaderModule.value(), nullptr);
        vkDestroyShaderModule(m_Device, fragShaderModule.value(), nullptr);
    }

    {
        m_LightPipelineLayout =
            PipelineLayoutBuilder::build(m_Device, { pushConstant }, { m_LightDescriptorLayout });

        std::optional<VkShaderModule> vertShaderModule =
            PipelineBuilder::createShaderModule(m_Device, "res/shaders/light.vert.spv");
        std::optional<VkShaderModule> fragShaderModule =
            PipelineBuilder::createShaderModule(m_Device, "res/shaders/light.frag.spv");

        m_LightPipeline =
            PipelineBuilder::start(m_Device, m_LightPipelineLayout)
                .setShaders(vertShaderModule.value(), fragShaderModule.value())
                .inputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .rasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                .setMultisampleNone()
                .disableBlending()
                .setColourAttachmentFormat(m_DrawImage.imageFormat)
                .setDepthFormat(m_DepthImage.imageFormat)
                .enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS)
                .build();

        vkDestroyShaderModule(m_Device, vertShaderModule.value(), nullptr);
        vkDestroyShaderModule(m_Device, fragShaderModule.value(), nullptr);
    }
}

void Engine::initTextures()
{
    m_BoxTexture.load(m_Device, m_Allocator, "res/textures/container.jpg",
                      VK_IMAGE_USAGE_SAMPLED_BIT);
    m_BoxTexture.createSampler(m_Device, VK_FILTER_LINEAR);

    m_FaceTexture.load(m_Device, m_Allocator, "res/textures/awesomeface.png",
                       VK_IMAGE_USAGE_SAMPLED_BIT);
    m_FaceTexture.createSampler(m_Device, VK_FILTER_LINEAR);
}

void Engine::createMaterials()
{
    std::vector<MaterialData> materials = {
        MaterialData{
                     .ambient = glm::vec3(0.3f),
                     .diffuse = glm::vec3(1.0f),
                     .specular = glm::vec4(1.0f, 1.0f, 1.0f, 32.0f),
                     }
    };

    size_t size = m_MaxMaterials * sizeof(MaterialData);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_MaterialDataBuffer[i].createBuffer(m_Allocator, size,
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             VMA_MEMORY_USAGE_GPU_ONLY);

        m_MaterialDataBuffer[i].pushData<MaterialData>(m_Allocator, materials);
    }
}

void Engine::createObjects()
{
    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),   glm::vec3(2.0f, -5.0f, -15.0f),
        glm::vec3(-1.5f, 2.2f, -2.5f), glm::vec3(-3.8f, 2.0f, -12.3f),
        glm::vec3(2.4f, 0.4f, -3.5f),  glm::vec3(-1.7f, -3.0f, -7.5f),
        glm::vec3(1.3f, 2.0f, -2.5f),  glm::vec3(1.5f, -2.0f, -2.5f),
        glm::vec3(1.5f, -0.2f, -1.5f), glm::vec3(-1.3f, -1.0f, -1.5f)
    };

    m_ObjectCount = cubePositions.size();

    size_t size = m_ObjectCount * sizeof(ObjectData);

    std::vector<ObjectData> models(m_ObjectCount);
    for (size_t i = 0; i < models.size(); i++)
    {
        glm::mat4 model{ 1.0f };
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, -0.3f, 0.5f));

        glm::mat4 rotation =
            glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(1.0f, -0.3f, 0.5f));

        models.at(i) = { .materialIndex = 0,
                         .colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                         .model = model,
                         .rotation = rotation };
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_ObjectDataBuffer[i].createBuffer(m_Allocator, size,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);

        m_ObjectDataBuffer[i].pushData<ObjectData>(m_Allocator, models);
    }
}

void Engine::createLights()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_LightDataBuffer[i].createBuffer(
            m_Allocator, sizeof(LightData) * m_MaxLights + sizeof(LightGeneralData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
    }

    uploadLightData();
}

void Engine::uploadLightData()
{
    float time = 0.001f * m_LightTime;
    glm::vec3 movingLightPosition = glm::vec3(10 * sin(time), 0.0f, 10 * cos(time));
    glm::vec3 movingLightColour =
        glm::vec3(fabs(cos(time) + sin(time)), fabs(cos(time)), fabs(sin(time)));

    std::vector<LightData> lights = {
  // { .position = glm::vec3(0.0f, -2.0f, 0.0f), .colour = glm::vec4(1.0f) },
        {.position = movingLightPosition,
         .diffuse = glm::vec3(movingLightColour * 0.2f),
         .specular = glm::vec3(1.0f)}
    };
    m_LightCount = lights.size();

    LightGeneralData generalData;
    generalData.lightCount = lights.size();
    generalData.ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);

    for (size_t i = 0; i < lights.size(); i++)
    {
        glm::mat4 model{ 1.0f };
        model = glm::translate(model, lights[i].position);
        model = glm::scale(model, glm::vec3(0.2f));
        lights[i].model = model;
    }

    size_t size = lights.size() * sizeof(LightData) + sizeof(LightGeneralData);

    AllocatedBuffer staging;
    staging.createBuffer(m_Allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(staging.allocationInfo.pMappedData, &generalData, sizeof(LightGeneralData));

    memcpy((char*)staging.allocationInfo.pMappedData + sizeof(LightGeneralData), lights.data(),
           lights.size() * sizeof(LightData));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_LightDataBuffer[i].pushFromBuffer(m_Allocator, staging, size);
    }

    staging.destroyBuffer(m_Allocator);
}

void Engine::initDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = MAX_FRAMES_IN_FLIGHT * 2                                                   },
        { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,        .descriptorCount = MAX_FRAMES_IN_FLIGHT * 3}
    };

    const uint32_t maxSets = MAX_FRAMES_IN_FLIGHT * 3;

    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCI.pPoolSizes = poolSizes.data();
    descriptorPoolCI.maxSets = maxSets;

    VK_CHECK(vkCreateDescriptorPool(m_Device, &descriptorPoolCI, nullptr, &m_DescriptorPool));
}

void Engine::initDescriptorSets()
{
    m_ObjectDescriptors =
        DescriptorSetBuilder::start(m_Device, m_DescriptorPool, MAX_FRAMES_IN_FLIGHT,
                                    m_ObjectDescriptorLayout)
            .addCombinedImageSampler(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     m_BoxTexture.imageView, m_BoxTexture.imageSampler.value())
            .addCombinedImageSampler(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     m_FaceTexture.imageView, m_FaceTexture.imageSampler.value())
            .addStorageBuffers(2, m_ObjectDataBuffer, 0, m_ObjectCount * sizeof(ObjectData))
            .build();

    m_LightDescriptors =
        DescriptorSetBuilder::start(m_Device, m_DescriptorPool, MAX_FRAMES_IN_FLIGHT,
                                    m_LightDescriptorLayout)
            .addStorageBuffers(0, m_LightDataBuffer, 0,
                               m_MaxLights * sizeof(LightData) + sizeof(LightGeneralData))
            .build();

    m_MaterialDescriptors =
        DescriptorSetBuilder::start(m_Device, m_DescriptorPool, MAX_FRAMES_IN_FLIGHT,
                                    m_MaterialDescriptorLayout)
            .addStorageBuffers(0, m_MaterialDataBuffer, 0, m_MaxMaterials * sizeof(MaterialData))
            .build();
}

void Engine::createMesh()
{
    std::vector<Vertex> vertices = {
  // Front: 0-3
        { .position = { -0.5f, -0.5f, 0.5f },
         .uv = { 0.0f, 0.0f },
         .normal = { 0.0f, 0.0f, 1.0f } },
        { .position = { 0.5f, -0.5f, 0.5f }, .uv = { 1.0f, 0.0f }, .normal = { 0.0f, 0.0f, 1.0f } },
        { .position = { -0.5f, 0.5f, 0.5f }, .uv = { 0.0f, 1.0f }, .normal = { 0.0f, 0.0f, 1.0f } },
        { .position = { 0.5f, 0.5f, 0.5f }, .uv = { 1.0f, 1.0f }, .normal = { 0.0f, 0.0f, 1.0f } },

 // Back: 4-7
        { .position = { 0.5f, -0.5f, -0.5f },
         .uv = { 0.0f, 0.0f },
         .normal = { 0.0f, 0.0f, -1.0f } },
        { .position = { -0.5f, -0.5f, -0.5f },
         .uv = { 1.0f, 0.0f },
         .normal = { 0.0f, 0.0f, -1.0f } },
        { .position = { 0.5f, 0.5f, -0.5f },
         .uv = { 0.0f, 1.0f },
         .normal = { 0.0f, 0.0f, -1.0f } },
        { .position = { -0.5f, 0.5f, -0.5f },
         .uv = { 1.0f, 1.0f },
         .normal = { 0.0f, 0.0f, -1.0f } },

 // Right: 8-11
        {
         .position = { 0.5f, -0.5f, 0.5f },
         .uv = { 0.0f, 0.0f },
         .normal = { 1.0f, 0.0f, 0.0f },
         },
        {
         .position = { 0.5f, -0.5f, -0.5f },
         .uv = { 1.0f, 0.0f },
         .normal = { 1.0f, 0.0f, 0.0f },
         },
        {
         .position = { 0.5f, 0.5f, 0.5f },
         .uv = { 0.0f, 1.0f },
         .normal = { 1.0f, 0.0f, 0.0f },
         },
        {
         .position = { 0.5f, 0.5f, -0.5f },
         .uv = { 1.0f, 1.0f },
         .normal = { 1.0f, 0.0f, 0.0f },
         },

 // Left: 12-15
        { .position = { -0.5f, -0.5f, -0.5f },
         .uv = { 0.0f, 0.0f },
         .normal = { -1.0f, 0.0f, 0.0f } },
        { .position = { -0.5f, -0.5f, 0.5f },
         .uv = { 1.0f, 0.0f },
         .normal = { -1.0f, 0.0f, 0.0f } },
        { .position = { -0.5f, 0.5f, -0.5f },
         .uv = { 0.0f, 1.0f },
         .normal = { -1.0f, 0.0f, 0.0f } },
        { .position = { -0.5f, 0.5f, 0.5f },
         .uv = { 1.0f, 1.0f },
         .normal = { -1.0f, 0.0f, 0.0f } },

 // Top: 16-19
        { .position = { -0.5f, -0.5f, -0.5f },
         .uv = { 0.0f, 0.0f },
         .normal = { 0.0f, -1.0f, 0.0f } },
        { .position = { 0.5f, -0.5f, -0.5f },
         .uv = { 1.0f, 0.0f },
         .normal = { 0.0f, -1.0f, 0.0f } },
        { .position = { -0.5f, -0.5f, 0.5f },
         .uv = { 0.0f, 1.0f },
         .normal = { 0.0f, -1.0f, 0.0f } },
        { .position = { 0.5f, -0.5f, 0.5f },
         .uv = { 1.0f, 1.0f },
         .normal = { 0.0f, -1.0f, 0.0f } },

 // Bottom: 20-23
        { .position = { -0.5f, 0.5f, 0.5f }, .uv = { 0.0f, 0.0f }, .normal = { 0.0f, 1.0f, 0.0f } },
        { .position = { 0.5f, 0.5f, 0.5f }, .uv = { 1.0f, 0.0f }, .normal = { 0.0f, 1.0f, 0.0f } },
        { .position = { -0.5f, 0.5f, -0.5f },
         .uv = { 0.0f, 1.0f },
         .normal = { 0.0f, 1.0f, 0.0f } },
        { .position = { 0.5f, 0.5f, -0.5f }, .uv = { 1.0f, 1.0f }, .normal = { 0.0f, 1.0f, 0.0f } },
    };

    std::vector<uint32_t> indices = {
        0,  1,  2,  1,  3,  2,  // Front
        4,  5,  6,  5,  7,  6,  // Back
        8,  9,  10, 9,  11, 10, // Right
        12, 13, 14, 13, 15, 14, // Left
        16, 17, 18, 17, 19, 18, // Top
        20, 21, 22, 21, 23, 22  // Bottom
    };
    m_BasicMesh.createMesh<Vertex>(m_Device, m_Allocator, indices, vertices);
}

FrameData& Engine::getCurrentFrame() { return m_Frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT]; }

void Engine::renderGeometry(VkCommandBuffer& cmd)
{
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_DrawImage.imageExtent.width;
    viewport.height = m_DrawImage.imageExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    scissor.extent.width = m_DrawImage.imageExtent.width;
    scissor.extent.height = m_DrawImage.imageExtent.height;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipelineLayout, 0, 1,
                            &m_LightDescriptors[m_CurrentFrame % 2], 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipelineLayout, 1, 1,
                            &m_ObjectDescriptors[m_CurrentFrame % 2], 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipelineLayout, 2, 1,
                            &m_MaterialDescriptors[m_CurrentFrame % 2], 0, nullptr);

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindIndexBuffer(cmd, m_BasicMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    VertexPushConstant pushConstantData;
    pushConstantData.view = m_Camera.getView();
    pushConstantData.proj = m_Camera.getPerspective(m_Window->getSize());
    pushConstantData.cameraPos = m_Camera.getPosition();
    pushConstantData.vertexBuffer = m_BasicMesh.vertexBufferAddress;

    vkCmdPushConstants(cmd, m_MeshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(VertexPushConstant), &pushConstantData);

    vkCmdDrawIndexed(cmd, m_BasicMesh.indexCount, m_ObjectCount, 0, 0, 0);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightPipelineLayout, 0, 1,
                            &m_LightDescriptors[m_CurrentFrame % 2], 0, nullptr);

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindIndexBuffer(cmd, m_BasicMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(cmd, m_LightPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(VertexPushConstant), &pushConstantData);

    vkCmdDrawIndexed(cmd, m_BasicMesh.indexCount, m_LightCount, 0, 0, 0);
}

void Engine::update()
{
    static auto previousTime = std::chrono::system_clock::now();
    auto newTime = std::chrono::system_clock::now();
    double dt =
        std::chrono::duration_cast<std::chrono::milliseconds>(newTime - previousTime).count();
    previousTime = newTime;
    m_Camera.update(dt);

    m_LightTime += dt;
    uploadLightData();
}

void Engine::render()
{
    VK_CHECK(vkWaitForFences(m_Device, 1, &getCurrentFrame().renderFence, true, 1e9));

    uint32_t swapchainImageIndex;
    {
        VkResult result =
            vkAcquireNextImageKHR(m_Device, m_Swapchain, 1e9, getCurrentFrame().swapchainSemaphore,
                                  nullptr, &swapchainImageIndex);
    }

    VK_CHECK(vkResetFences(m_Device, 1, &getCurrentFrame().renderFence));

    VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo commandBufferBI{};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBI.pNext = nullptr;
    commandBufferBI.pInheritanceInfo = nullptr;
    commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmd, &commandBufferBI));

    AllocatedImage::transition(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clearColour = {
        {0.2f, 0.2f, 0.2f, 1.0f}
    };
    VkImageSubresourceRange range{};
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCmdClearColorImage(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearColour, 1, &range);

    AllocatedImage::transition(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_GENERAL,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    AllocatedImage::transition(cmd, m_DepthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo colourAI{};
    colourAI.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAI.pNext = nullptr;
    colourAI.imageView = m_DrawImage.imageView;
    colourAI.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    colourAI.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colourAI.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colourAI.clearValue.color = {
        {0.2f, 0.2f, 0.2f, 1.0f}
    };

    VkRenderingAttachmentInfo depthAI{};
    depthAI.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAI.pNext = nullptr;
    depthAI.imageView = m_DepthImage.imageView;
    depthAI.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAI.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAI.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAI.clearValue.depthStencil.depth = 1.0f;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;
    renderInfo.flags = 0;
    renderInfo.renderArea =
        VkRect2D({ 0, 0 }, { m_DrawImage.imageExtent.width, m_DrawImage.imageExtent.height });
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colourAI;
    renderInfo.pDepthAttachment = &depthAI;
    renderInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(cmd, &renderInfo);

    renderGeometry(cmd);

    vkCmdEndRendering(cmd);

    AllocatedImage::transition(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    AllocatedImage::transition(cmd, m_SwapchainImages[swapchainImageIndex],
                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkExtent2D drawExtent = { m_DrawImage.imageExtent.width, m_DrawImage.imageExtent.height };
    AllocatedImage::copyImgToImg(cmd, m_DrawImage.image, m_SwapchainImages[swapchainImageIndex],
                                 drawExtent, m_SwapchainImageExtent);

    AllocatedImage::transition(cmd, m_SwapchainImages[swapchainImageIndex],
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo commandBufferSI{};
    commandBufferSI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferSI.pNext = nullptr;
    commandBufferSI.commandBuffer = cmd;
    commandBufferSI.deviceMask = 0;

    VkSemaphoreSubmitInfo waitSI{};
    waitSI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSI.pNext = nullptr;
    waitSI.semaphore = getCurrentFrame().swapchainSemaphore;
    waitSI.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    waitSI.deviceIndex = 0;
    waitSI.value = 1;

    VkSemaphoreSubmitInfo signalSI{};
    signalSI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSI.pNext = nullptr;
    signalSI.semaphore = getCurrentFrame().renderSemaphore;
    signalSI.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signalSI.deviceIndex = 0;
    signalSI.value = 1;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSI;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSI;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferSI;

    VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, getCurrentFrame().renderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
    presentInfo.pImageIndices = &swapchainImageIndex;

    {
        VkResult result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
    }

    m_CurrentFrame++;
}

void Engine::mainLoop()
{
    while (!m_Window->shouldClose())
    {
        m_Window->getEvents();

        update();

        render();

        m_Window->swapBuffers();
    }
}
