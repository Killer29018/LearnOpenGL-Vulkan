#include "Pipeline.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "ErrorCheck.hpp"

#include <fstream>
#include <iostream>
#include <vector>

VkPipelineLayout
PipelineLayoutBuilder::build(VkDevice device,
                             std::initializer_list<VkPushConstantRange> pushConstants,
                             std::initializer_list<VkDescriptorSetLayout> descriptorLayouts)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
    pipelineLayoutCI.pSetLayouts = std::data(descriptorLayouts);
    pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    pipelineLayoutCI.pPushConstantRanges = std::data(pushConstants);

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &layout));

    return layout;
}

VkPipelineLayout PipelineLayoutBuilder ::build(VkDevice device,
                                               std::span<VkPushConstantRange> pushConstants,
                                               std::span<VkDescriptorSetLayout> descriptorLayouts)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
    pipelineLayoutCI.pSetLayouts = descriptorLayouts.data();
    pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    pipelineLayoutCI.pPushConstantRanges = pushConstants.data();

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &layout));

    return layout;
}

PipelineBuilder PipelineBuilder::start(VkDevice device, VkPipelineLayout layout)
{
    PipelineBuilder builder(device, layout);
    return builder;
}

PipelineBuilder& PipelineBuilder::setShaders(VkShaderModule vertShaderModule,
                                             VkShaderModule fragShaderModule)
{
    m_ShaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                               .pNext = nullptr,
                               .flags = 0,
                               .stage = VK_SHADER_STAGE_VERTEX_BIT,
                               .module = vertShaderModule,
                               .pName = "main" });
    m_ShaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                               .pNext = nullptr,
                               .flags = 0,
                               .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                               .module = fragShaderModule,
                               .pName = "main" });

    return *this;
}

PipelineBuilder& PipelineBuilder::setShaders(VkShaderModule vertShaderModule,
                                             VkShaderModule geoShaderModule,
                                             VkShaderModule fragShaderModule)
{
    setShaders(vertShaderModule, fragShaderModule);
    m_ShaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                               .pNext = nullptr,
                               .flags = 0,
                               .stage = VK_SHADER_STAGE_GEOMETRY_BIT,
                               .module = geoShaderModule,
                               .pName = "main" });

    return *this;
}

PipelineBuilder& PipelineBuilder::inputAssembly(VkPrimitiveTopology topology)
{
    m_InputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssemblyCI.pNext = nullptr;
    m_InputAssemblyCI.flags = 0;
    m_InputAssemblyCI.topology = topology;
    m_InputAssemblyCI.primitiveRestartEnable = VK_FALSE;

    return *this;
}

PipelineBuilder& PipelineBuilder::rasterizer(VkPolygonMode mode, VkCullModeFlags cullMode,
                                             VkFrontFace frontFace)
{
    m_RasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizerCI.pNext = nullptr;
    m_RasterizerCI.flags = 0;
    m_RasterizerCI.depthClampEnable = VK_FALSE;
    m_RasterizerCI.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizerCI.polygonMode = mode;
    m_RasterizerCI.cullMode = cullMode;
    m_RasterizerCI.frontFace = frontFace;
    m_RasterizerCI.depthBiasEnable = VK_FALSE;
    m_RasterizerCI.depthBiasConstantFactor = 0.0f;
    m_RasterizerCI.depthBiasClamp = 0.0f;
    m_RasterizerCI.depthBiasSlopeFactor = 0.0f;
    m_RasterizerCI.lineWidth = 1.0f;

    return *this;
}

PipelineBuilder& PipelineBuilder::setMultisampleNone()
{
    m_MultisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisampleCI.pNext = nullptr;
    m_MultisampleCI.flags = 0;
    m_MultisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_MultisampleCI.sampleShadingEnable = VK_FALSE;
    m_MultisampleCI.minSampleShading = 1.0f;
    m_MultisampleCI.pSampleMask = nullptr;
    m_MultisampleCI.alphaToCoverageEnable = VK_FALSE;
    m_MultisampleCI.alphaToOneEnable = VK_FALSE;

    return *this;
}

PipelineBuilder& PipelineBuilder::disableBlending()
{
    m_ColourBlendAS.blendEnable = VK_FALSE;
    m_ColourBlendAS.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    return *this;
}

PipelineBuilder& PipelineBuilder::setColourAttachmentFormat(VkFormat format)
{
    m_ColourAttachmentFormat = format;

    m_RenderCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    m_RenderCI.pNext = nullptr;
    m_RenderCI.colorAttachmentCount = 1;
    m_RenderCI.pColorAttachmentFormats = &m_ColourAttachmentFormat;

    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthFormat(VkFormat format)
{
    m_RenderCI.depthAttachmentFormat = format;

    return *this;
}

PipelineBuilder& PipelineBuilder::disableDepthTest()
{
    m_DepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilCI.pNext = nullptr;
    m_DepthStencilCI.flags = 0;
    m_DepthStencilCI.depthTestEnable = VK_FALSE;
    m_DepthStencilCI.depthWriteEnable = VK_FALSE;
    m_DepthStencilCI.depthCompareOp = VK_COMPARE_OP_NEVER;
    m_DepthStencilCI.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilCI.stencilTestEnable = VK_FALSE;
    m_DepthStencilCI.front = {};
    m_DepthStencilCI.back = {};
    m_DepthStencilCI.minDepthBounds = 0.0f;
    m_DepthStencilCI.maxDepthBounds = 1.0f;

    return *this;
}
PipelineBuilder& PipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp compareOp)
{
    m_DepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilCI.pNext = nullptr;
    m_DepthStencilCI.flags = 0;
    m_DepthStencilCI.depthTestEnable = VK_TRUE;
    m_DepthStencilCI.depthWriteEnable = depthWriteEnable;
    m_DepthStencilCI.depthCompareOp = compareOp;
    m_DepthStencilCI.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilCI.stencilTestEnable = VK_FALSE;
    m_DepthStencilCI.front = {};
    m_DepthStencilCI.back = {};
    m_DepthStencilCI.minDepthBounds = 0.0f;
    m_DepthStencilCI.maxDepthBounds = 1.0f;

    return *this;
}

VkPipeline PipelineBuilder::build()
{
    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.pNext = nullptr;
    viewportStateCI.flags = 0;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.pViewports = nullptr;
    viewportStateCI.scissorCount = 1;
    viewportStateCI.pScissors = nullptr;

    VkPipelineColorBlendStateCreateInfo colourBlendingCI{};
    colourBlendingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendingCI.pNext = nullptr;
    colourBlendingCI.logicOpEnable = VK_FALSE;
    colourBlendingCI.logicOp = VK_LOGIC_OP_COPY;
    colourBlendingCI.attachmentCount = 1;
    colourBlendingCI.pAttachments = &m_ColourBlendAS;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.pNext = nullptr;
    vertexInputStateCI.flags = 0;
    vertexInputStateCI.vertexBindingDescriptionCount = 0;
    vertexInputStateCI.pVertexBindingDescriptions = nullptr;
    vertexInputStateCI.vertexAttributeDescriptionCount = 0;
    vertexInputStateCI.pVertexAttributeDescriptions = 0;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCI.pDynamicStates = dynamicStates.data();

    VkPipelineTessellationStateCreateInfo tessellationStateCI{};
    tessellationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationStateCI.pNext = nullptr;
    tessellationStateCI.flags = 0;
    tessellationStateCI.patchControlPoints = 0;

    VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
    graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCI.pNext = &m_RenderCI;
    graphicsPipelineCI.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
    graphicsPipelineCI.pStages = m_ShaderStages.data();
    graphicsPipelineCI.pVertexInputState = &vertexInputStateCI;
    graphicsPipelineCI.pInputAssemblyState = &m_InputAssemblyCI;
    graphicsPipelineCI.pViewportState = &viewportStateCI;
    graphicsPipelineCI.pRasterizationState = &m_RasterizerCI;
    graphicsPipelineCI.pMultisampleState = &m_MultisampleCI;
    graphicsPipelineCI.pColorBlendState = &colourBlendingCI;
    graphicsPipelineCI.pDepthStencilState = &m_DepthStencilCI;
    graphicsPipelineCI.pDynamicState = &dynamicStateCI;
    graphicsPipelineCI.layout = m_PipelineLayout;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr,
                                  &pipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline\n";
        pipeline = VK_NULL_HANDLE;
    }

    return pipeline;
}

std::optional<VkShaderModule> PipelineBuilder::createShaderModule(VkDevice device,

                                                                  std::filesystem::path filePath)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return {};
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo shaderModuleCI{};
    shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCI.pNext = nullptr;
    shaderModuleCI.codeSize = buffer.size() * sizeof(uint32_t);
    shaderModuleCI.pCode = buffer.data();

    VkShaderModule module;
    VkResult result = vkCreateShaderModule(device, &shaderModuleCI, nullptr, &module);
    if (result != VK_SUCCESS)
    {
        std::cout << string_VkResult(result) << "\n";
        return {};
    }
    return module;
}

PipelineBuilder::PipelineBuilder(VkDevice device, VkPipelineLayout layout)
    : m_Device{ device }, m_PipelineLayout{ layout }, m_ShaderStages{}, m_ColourAttachmentFormat{},
      m_InputAssemblyCI{}, m_RasterizerCI{}, m_ColourBlendAS{}, m_MultisampleCI{},
      m_DepthStencilCI{}, m_RenderCI{}
{
    m_RenderCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    m_RenderCI.pNext = nullptr;
}
