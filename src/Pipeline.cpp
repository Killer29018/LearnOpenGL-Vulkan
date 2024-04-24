#include "Pipeline.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "ErrorCheck.hpp"

#include <fstream>
#include <iostream>
#include <vector>

std::vector<VkPushConstantRange> PipelineBuilder::s_PushConstants;
std::vector<VkDescriptorSetLayout> PipelineBuilder::s_DescriptorLayouts;
std::vector<VkPipelineShaderStageCreateInfo> PipelineBuilder::s_ShaderStages;

VkFormat PipelineBuilder::s_ColourAttachmentFormat;
VkPipelineInputAssemblyStateCreateInfo PipelineBuilder::s_InputAssemblyCI;
VkPipelineRasterizationStateCreateInfo PipelineBuilder::s_RasterizerCI;
VkPipelineColorBlendAttachmentState PipelineBuilder::s_ColourBlendAS;
VkPipelineMultisampleStateCreateInfo PipelineBuilder::s_MultisampleCI;
VkPipelineDepthStencilStateCreateInfo PipelineBuilder::s_DepthStencilCI;
VkPipelineRenderingCreateInfo PipelineBuilder::s_RenderCI;

void PipelineBuilder::reset()
{
    s_PushConstants.clear();
    s_DescriptorLayouts.clear();
    s_ShaderStages.clear();

    s_ColourAttachmentFormat = {};
    s_InputAssemblyCI = {};
    s_RasterizerCI = {};
    s_ColourBlendAS = {};
    s_MultisampleCI = {};
    s_DepthStencilCI = {};
    s_RenderCI = {};
}

void PipelineBuilder::addPushConstant(VkPushConstantRange pushConstant)
{
    s_PushConstants.push_back(pushConstant);
}

void PipelineBuilder::addDescriptorLayout(VkDescriptorSetLayout descriptorLayout)
{
    s_DescriptorLayouts.push_back(descriptorLayout);
}

void PipelineBuilder::setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule)
{
    s_ShaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                               .pNext = nullptr,
                               .flags = 0,
                               .stage = VK_SHADER_STAGE_VERTEX_BIT,
                               .module = vertShaderModule,
                               .pName = "main" });
    s_ShaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                               .pNext = nullptr,
                               .flags = 0,
                               .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                               .module = fragShaderModule,
                               .pName = "main" });
}

void PipelineBuilder::inputAssembly(VkPrimitiveTopology topology)
{
    s_InputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    s_InputAssemblyCI.pNext = nullptr;
    s_InputAssemblyCI.flags = 0;
    s_InputAssemblyCI.topology = topology;
    s_InputAssemblyCI.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::rasterizer(VkPolygonMode mode, VkCullModeFlags cullMode,
                                 VkFrontFace frontFace)
{
    s_RasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    s_RasterizerCI.pNext = nullptr;
    s_RasterizerCI.flags = 0;
    s_RasterizerCI.depthClampEnable = VK_FALSE;
    s_RasterizerCI.rasterizerDiscardEnable = VK_FALSE;
    s_RasterizerCI.polygonMode = mode;
    s_RasterizerCI.cullMode = cullMode;
    s_RasterizerCI.frontFace = frontFace;
    s_RasterizerCI.depthBiasEnable = VK_FALSE;
    s_RasterizerCI.depthBiasConstantFactor = 0.0f;
    s_RasterizerCI.depthBiasClamp = 0.0f;
    s_RasterizerCI.depthBiasSlopeFactor = 0.0f;
    s_RasterizerCI.lineWidth = 1.0f;
}

void PipelineBuilder::setMultisampleNone()
{
    s_MultisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    s_MultisampleCI.pNext = nullptr;
    s_MultisampleCI.flags = 0;
    s_MultisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    s_MultisampleCI.sampleShadingEnable = VK_FALSE;
    s_MultisampleCI.minSampleShading = 1.0f;
    s_MultisampleCI.pSampleMask = nullptr;
    s_MultisampleCI.alphaToCoverageEnable = VK_FALSE;
    s_MultisampleCI.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::disableBlending()
{
    s_ColourBlendAS.blendEnable = VK_FALSE;
    s_ColourBlendAS.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

void PipelineBuilder::setColourAttachmentFormat(VkFormat format)
{
    s_ColourAttachmentFormat = format;

    s_RenderCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    s_RenderCI.pNext = nullptr;
    s_RenderCI.colorAttachmentCount = 1;
    s_RenderCI.pColorAttachmentFormats = &s_ColourAttachmentFormat;
}

void PipelineBuilder::setDepthFormat(VkFormat format) { s_RenderCI.depthAttachmentFormat = format; }

void PipelineBuilder::disableDepthTest()
{
    s_DepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    s_DepthStencilCI.pNext = nullptr;
    s_DepthStencilCI.flags = 0;
    s_DepthStencilCI.depthTestEnable = VK_FALSE;
    s_DepthStencilCI.depthWriteEnable = VK_FALSE;
    s_DepthStencilCI.depthCompareOp = VK_COMPARE_OP_NEVER;
    s_DepthStencilCI.depthBoundsTestEnable = VK_FALSE;
    s_DepthStencilCI.stencilTestEnable = VK_FALSE;
    s_DepthStencilCI.front = {};
    s_DepthStencilCI.back = {};
    s_DepthStencilCI.minDepthBounds = 0.0f;
    s_DepthStencilCI.maxDepthBounds = 1.0f;
}
void PipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp compareOp)
{
    s_DepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    s_DepthStencilCI.pNext = nullptr;
    s_DepthStencilCI.flags = 0;
    s_DepthStencilCI.depthTestEnable = VK_TRUE;
    s_DepthStencilCI.depthWriteEnable = depthWriteEnable;
    s_DepthStencilCI.depthCompareOp = compareOp;
    s_DepthStencilCI.depthBoundsTestEnable = VK_FALSE;
    s_DepthStencilCI.stencilTestEnable = VK_FALSE;
    s_DepthStencilCI.front = {};
    s_DepthStencilCI.back = {};
    s_DepthStencilCI.minDepthBounds = 0.0f;
    s_DepthStencilCI.maxDepthBounds = 1.0f;
}

Pipeline PipelineBuilder::build(VkDevice device)
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
    colourBlendingCI.pAttachments = &s_ColourBlendAS;

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

    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(s_DescriptorLayouts.size());
    pipelineLayoutCI.pSetLayouts = s_DescriptorLayouts.data();
    pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(s_PushConstants.size());
    pipelineLayoutCI.pPushConstantRanges = s_PushConstants.data();

    Pipeline pipeline;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipeline.pipelineLayout));

    VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
    graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCI.pNext = &s_RenderCI;
    graphicsPipelineCI.stageCount = static_cast<uint32_t>(s_ShaderStages.size());
    graphicsPipelineCI.pStages = s_ShaderStages.data();
    graphicsPipelineCI.pVertexInputState = &vertexInputStateCI;
    graphicsPipelineCI.pInputAssemblyState = &s_InputAssemblyCI;
    graphicsPipelineCI.pViewportState = &viewportStateCI;
    graphicsPipelineCI.pRasterizationState = &s_RasterizerCI;
    graphicsPipelineCI.pMultisampleState = &s_MultisampleCI;
    graphicsPipelineCI.pColorBlendState = &colourBlendingCI;
    graphicsPipelineCI.pDepthStencilState = &s_DepthStencilCI;
    graphicsPipelineCI.pDynamicState = &dynamicStateCI;
    graphicsPipelineCI.layout = pipeline.pipelineLayout;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr,
                                  &pipeline.pipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline\n";
        pipeline.pipeline = VK_NULL_HANDLE;
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
