#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>

class PipelineLayoutBuilder
{
  public:
    static VkPipelineLayout build(VkDevice device,
                                  std::initializer_list<VkPushConstantRange> pushConstants,
                                  std::initializer_list<VkDescriptorSetLayout> descriptorLayouts);
    static VkPipelineLayout build(VkDevice device, std::span<VkPushConstantRange> pushConstants,
                                  std::span<VkDescriptorSetLayout> descriptorLayouts);
};

class PipelineBuilder
{
  public:
    static PipelineBuilder start(VkDevice device, VkPipelineLayout layout);

    PipelineBuilder& setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
    PipelineBuilder& setShaders(VkShaderModule vertShaderModule, VkShaderModule geoShaderModule,
                                VkShaderModule fragShaderModule);

    PipelineBuilder& inputAssembly(VkPrimitiveTopology topology);
    PipelineBuilder& rasterizer(VkPolygonMode mode, VkCullModeFlags cullMode,
                                VkFrontFace frontFace);
    PipelineBuilder& setMultisampleNone();

    PipelineBuilder& disableBlending();

    PipelineBuilder& addColourAttachmentFormat(VkFormat format);
    PipelineBuilder& addColourAttachmentFormats(std::initializer_list<VkFormat> formats);
    PipelineBuilder& addColourAttachmentFormats(std::span<VkFormat> formats);

    PipelineBuilder& setDepthFormat(VkFormat format);

    PipelineBuilder& disableDepthTest();
    PipelineBuilder& enableDepthTest(bool depthWriteEnable, VkCompareOp compareOp);

    VkPipeline build();

    static std::optional<VkShaderModule> createShaderModule(VkDevice device,
                                                            std::filesystem::path filePath);

  private:
    PipelineBuilder(VkDevice device, VkPipelineLayout layout);

  private:
    VkDevice m_Device;
    VkPipelineLayout m_PipelineLayout;

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    std::vector<VkFormat> m_ColourFormats;

    VkFormat m_ColourAttachmentFormat;
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyCI;
    VkPipelineRasterizationStateCreateInfo m_RasterizerCI;
    VkPipelineColorBlendAttachmentState m_ColourBlendAS;
    VkPipelineMultisampleStateCreateInfo m_MultisampleCI;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilCI;
    VkPipelineRenderingCreateInfo m_RenderCI;
};
