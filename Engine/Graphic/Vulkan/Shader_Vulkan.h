#pragma once
#include "Graphic/Vulkan/Common_Vulkan.h"
#include "Graphic/Shader.h"

namespace graphic {
class Shader_Vulkan : public Shader {
    friend class Device_Vulkan;
    friend class CommandList_Vulkan;
    friend class PipeLine_Vulkan;
    friend struct PipeLineLayout;
public:
    Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize);
    ~Shader_Vulkan();
private:
    Device_Vulkan* device_;
    VkShaderModule shaderModule_;
    VkPipelineShaderStageCreateInfo stageInfo_;

    // Layout info
    std::vector<VkDescriptorSetLayoutBinding> bindings_[DESCRIPTOR_SET_MAX_NUM];
    // std::vector<VkImageViewType> bindingViews_[DESCRIPTOR_SET_MAX_NUM];
    VkPushConstantRange pushConstant_;
};

CONVERT_TO_VULKAN_INTERNAL(Shader)
}