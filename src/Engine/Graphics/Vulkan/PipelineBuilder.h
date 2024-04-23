#pragma once
#include "pch.h"

class PipelineBuilder {
public:
    PipelineBuilder() { Clear(); }

    void Clear();
    VkPipeline BuildPipeline();

    // settings
    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setMultiSamplingNone();
    void setColorAttachmentFormat(VkFormat format);
    void setDepthAttachmentFormat(VkFormat format);
    void disableBlending();
    void disableDepthtest();
    void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
    void enableBlendingAdditive();
    void enableBlendingAlphaBlend();
    void setDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layout);

public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
   
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo renderInfo;
    VkFormat colorAttachmentformat;
};
