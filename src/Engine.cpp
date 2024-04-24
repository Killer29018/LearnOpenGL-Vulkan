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

    m_Window->attachObserver(this);

    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructures();

    ImmediateSubmit::init(m_Device, m_GraphicsQueue, m_GraphicsQueueFamily);

    initDescriptorSetLayouts();

    initPipelines();

    initTextures();

    createObjects();

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
            std::cout << "KEY PRESS\n";
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
    vkDestroyDescriptorSetLayout(m_Device, m_MainDescriptorLayout, nullptr);

    for (size_t i = 0; i < m_ObjectDataBuffer.size(); i++)
    {
        m_ObjectDataBuffer[i].destroyBuffer(m_Allocator);
    }

    m_FaceTexture.destroy(m_Device, m_Allocator);
    m_BoxTexture.destroy(m_Device, m_Allocator);

    vkDestroyPipeline(m_Device, m_BasicPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_BasicPipeline.pipelineLayout, nullptr);
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
    vkb::PhysicalDevice vkbPhysicalDevice = selector.set_minimum_version(1, 3)
                                                .set_required_features_13(features13)
                                                .set_required_features_12(features12)
                                                .set_required_features_11(features11)
                                                .set_required_features(features)
                                                .set_surface(m_Surface)
                                                .select()
                                                .value();

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
    VkDescriptorSetLayoutBinding boxBinding{};
    boxBinding.binding = 0;
    boxBinding.descriptorCount = 1;
    boxBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    boxBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    boxBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding faceBinding{};
    faceBinding.binding = 1;
    faceBinding.descriptorCount = 1;
    faceBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    faceBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    faceBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bufferBinding{};
    bufferBinding.binding = 2;
    bufferBinding.descriptorCount = 1;
    bufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bufferBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> bindings{ boxBinding, faceBinding, bufferBinding };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorSetLayoutCI.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutCI, nullptr,
                                         &m_MainDescriptorLayout));
}

void Engine::initPipelines()
{
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(VertexPushConstant);

    std::optional<VkShaderModule> vertShaderModule =
        PipelineBuilder::createShaderModule(m_Device, "res/shaders/mesh.vert.spv");

    std::optional<VkShaderModule> fragShaderModule =
        PipelineBuilder::createShaderModule(m_Device, "res/shaders/mesh.frag.spv");

    PipelineBuilder::reset();
    PipelineBuilder::addPushConstant(pushConstant);
    PipelineBuilder::addDescriptorLayout(m_MainDescriptorLayout);
    PipelineBuilder::setShaders(vertShaderModule.value(), fragShaderModule.value());
    PipelineBuilder::inputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    PipelineBuilder::rasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
                                VK_FRONT_FACE_CLOCKWISE);
    PipelineBuilder::setMultisampleNone();
    PipelineBuilder::disableBlending();
    PipelineBuilder::setColourAttachmentFormat(m_DrawImage.imageFormat);
    PipelineBuilder::setDepthFormat(m_DepthImage.imageFormat);
    PipelineBuilder::enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS);

    m_BasicPipeline = PipelineBuilder::build(m_Device);

    vkDestroyShaderModule(m_Device, vertShaderModule.value(), nullptr);
    vkDestroyShaderModule(m_Device, fragShaderModule.value(), nullptr);
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

void Engine::createObjects()
{
    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f, -0.0f, 0.0f),  glm::vec3(2.0f, -5.0f, -15.0f),
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

        models.at(i) = { .model = model };
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_ObjectDataBuffer[i].createBuffer(m_Allocator, size,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);
    }

    AllocatedBuffer stagingBuffer;
    stagingBuffer.createBuffer(m_Allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(stagingBuffer.allocationInfo.pMappedData, models.data(), size);

    ImmediateSubmit::submit([&](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = size;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_ObjectDataBuffer[i].buffer, 1, &copy);
        }
    });

    stagingBuffer.destroyBuffer(m_Allocator);
}

void Engine::initDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2},
        { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)    }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCI.pPoolSizes = poolSizes.data();
    descriptorPoolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VK_CHECK(vkCreateDescriptorPool(m_Device, &descriptorPoolCI, nullptr, &m_DescriptorPool));
}

void Engine::initDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT,
                                                            m_MainDescriptorLayout);

    VkDescriptorSetAllocateInfo descriptorSetAI{};
    descriptorSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAI.descriptorPool = m_DescriptorPool;
    descriptorSetAI.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    descriptorSetAI.pSetLayouts = descriptorSetLayouts.data();

    VK_CHECK(vkAllocateDescriptorSets(m_Device, &descriptorSetAI, m_MainDescriptors.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo descriptorII_Box{};
        descriptorII_Box.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorII_Box.imageView = m_BoxTexture.imageView;
        descriptorII_Box.sampler = m_BoxTexture.imageSampler.value();

        VkDescriptorImageInfo descriptorII_Face{};
        descriptorII_Face.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorII_Face.imageView = m_FaceTexture.imageView;
        descriptorII_Face.sampler = m_FaceTexture.imageSampler.value();

        VkDescriptorBufferInfo descriptorBI{};
        descriptorBI.buffer = m_ObjectDataBuffer[i].buffer;
        descriptorBI.offset = 0;
        descriptorBI.range = m_ObjectCount * sizeof(ObjectData);

        std::vector<VkWriteDescriptorSet> descriptorWrite{};
        descriptorWrite.push_back(
            VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = m_MainDescriptors[i],
                                  .dstBinding = 0,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .pImageInfo = &descriptorII_Box,
                                  .pBufferInfo = nullptr,
                                  .pTexelBufferView = nullptr });
        descriptorWrite.push_back(
            VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = m_MainDescriptors[i],
                                  .dstBinding = 1,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .pImageInfo = &descriptorII_Face,
                                  .pBufferInfo = nullptr,
                                  .pTexelBufferView = nullptr });
        descriptorWrite.push_back(
            VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = m_MainDescriptors[i],
                                  .dstBinding = 2,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  .pImageInfo = nullptr,
                                  .pBufferInfo = &descriptorBI,
                                  .pTexelBufferView = nullptr });

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrite.size()),
                               descriptorWrite.data(), 0, nullptr);
    }
}

void Engine::createMesh()
{
    std::vector<Vertex> vertices = {
  // Front: 0-3
        {.position = { -0.5f, -0.5f, 0.5f },   .uv = { 0.0f, 0.0f }},
        { .position = { 0.5f, -0.5f, 0.5f },   .uv = { 1.0f, 0.0f }},
        { .position = { -0.5f, 0.5f, 0.5f },   .uv = { 0.0f, 1.0f }},
        { .position = { 0.5f, 0.5f, 0.5f },    .uv = { 1.0f, 1.0f }},

 // Back: 4-7
        { .position = { 0.5f, -0.5f, -0.5f },  .uv = { 0.0f, 0.0f }},
        { .position = { -0.5f, -0.5f, -0.5f }, .uv = { 1.0f, 0.0f }},
        { .position = { 0.5f, 0.5f, -0.5f },   .uv = { 0.0f, 1.0f }},
        { .position = { -0.5f, 0.5f, -0.5f },  .uv = { 1.0f, 1.0f }},

 // Right: 8-11
        { .position = { 0.5f, -0.5f, 0.5f },   .uv = { 0.0f, 0.0f }},
        { .position = { 0.5f, -0.5f, -0.5f },  .uv = { 1.0f, 0.0f }},
        { .position = { 0.5f, 0.5f, 0.5f },    .uv = { 0.0f, 1.0f }},
        { .position = { 0.5f, 0.5f, -0.5f },   .uv = { 1.0f, 1.0f }},

 // Left: 12-15
        { .position = { -0.5f, -0.5f, -0.5f }, .uv = { 0.0f, 0.0f }},
        { .position = { -0.5f, -0.5f, 0.5f },  .uv = { 1.0f, 0.0f }},
        { .position = { -0.5f, 0.5f, -0.5f },  .uv = { 0.0f, 1.0f }},
        { .position = { -0.5f, 0.5f, 0.5f },   .uv = { 1.0f, 1.0f }},

 // Top: 16-19
        { .position = { -0.5f, -0.5f, -0.5f }, .uv = { 0.0f, 0.0f }},
        { .position = { 0.5f, -0.5f, -0.5f },  .uv = { 1.0f, 0.0f }},
        { .position = { -0.5f, -0.5f, 0.5f },  .uv = { 0.0f, 1.0f }},
        { .position = { 0.5f, -0.5f, 0.5f },   .uv = { 1.0f, 1.0f }},

 // Bottom: 20-23
        { .position = { -0.5f, 0.5f, 0.5f },   .uv = { 0.0f, 0.0f }},
        { .position = { 0.5f, 0.5f, 0.5f },    .uv = { 1.0f, 0.0f }},
        { .position = { -0.5f, 0.5f, -0.5f },  .uv = { 0.0f, 1.0f }},
        { .position = { 0.5f, 0.5f, -0.5f },   .uv = { 1.0f, 1.0f }},
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
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BasicPipeline.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BasicPipeline.pipelineLayout, 0,
                            1, &m_MainDescriptors[m_CurrentFrame % 2], 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_DrawImage.imageExtent.width;
    viewport.height = m_DrawImage.imageExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    scissor.extent.width = m_DrawImage.imageExtent.width;
    scissor.extent.height = m_DrawImage.imageExtent.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindIndexBuffer(cmd, m_BasicMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    VertexPushConstant pushConstantData;
    pushConstantData.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    pushConstantData.proj = glm::perspective(
        glm::radians(70.0f), (float)m_Window->getSize().x / (float)m_Window->getSize().y, 0.01f,
        1000.0f);
    pushConstantData.vertexBuffer = m_BasicMesh.vertexBufferAddress;

    vkCmdPushConstants(cmd, m_BasicPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(VertexPushConstant), &pushConstantData);

    vkCmdDrawIndexed(cmd, m_BasicMesh.indexCount, m_ObjectCount, 0, 0, 0);
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

        // glfwSetWindowShouldClose(m_Window->getWindow(), true);

        render();

        m_Window->swapBuffers();
    }
}
