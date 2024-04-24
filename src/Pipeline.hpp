#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <optional>
#include <vector>

struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

class PipelineBuilder
{
  public:
    static void reset();

    static void addPushConstant(VkPushConstantRange pushConstant);
    static void addDescriptorLayout(VkDescriptorSetLayout descriptorLayout);

    static void setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
    static void inputAssembly(VkPrimitiveTopology topology);
    static void rasterizer(VkPolygonMode mode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    static void setMultisampleNone();

    static void disableBlending();

    static void setColourAttachmentFormat(VkFormat format);
    static void setDepthFormat(VkFormat format);

    static void disableDepthTest();
    static void enableDepthTest(bool depthWriteEnable, VkCompareOp compareOp);

    static Pipeline build(VkDevice device);

    static std::optional<VkShaderModule> createShaderModule(VkDevice device,
                                                            std::filesystem::path filePath);

  private:
    static std::vector<VkPushConstantRange> s_PushConstants;
    static std::vector<VkDescriptorSetLayout> s_DescriptorLayouts;
    static std::vector<VkPipelineShaderStageCreateInfo> s_ShaderStages;

    static VkFormat s_ColourAttachmentFormat;
    static VkPipelineInputAssemblyStateCreateInfo s_InputAssemblyCI;
    static VkPipelineRasterizationStateCreateInfo s_RasterizerCI;
    static VkPipelineColorBlendAttachmentState s_ColourBlendAS;
    static VkPipelineMultisampleStateCreateInfo s_MultisampleCI;
    static VkPipelineDepthStencilStateCreateInfo s_DepthStencilCI;
    static VkPipelineRenderingCreateInfo s_RenderCI;
};
