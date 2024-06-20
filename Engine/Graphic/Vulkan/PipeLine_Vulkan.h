#pragma once
#include "Graphic/Vulkan/Common_Vulkan.h"
#include "Graphic/PipeLine.h"
#include "Graphic/Shader.h"

namespace graphic {
    
// This class is Cached in Device_Vulkan
struct PipeLineLayout {
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; 
    VkDescriptorSetLayout setLayouts[Shader::SHADER_RESOURCE_SET_MAX_NUM]; 
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings[Shader::SHADER_RESOURCE_SET_MAX_NUM];
    std::vector<VkImageViewType> imageViewTypes[Shader::SHADER_RESOURCE_SET_MAX_NUM];
    VkPushConstantRange pushConstant = {};
    uint32_t setLayoutMask = 0;

    size_t hash = 0;
};

class PipeLine_Vulkan : public PipeLine {
    friend class Device_Vulkan;
    friend class CommandList_Vulkan;
public:
    PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc);
    PipeLine_Vulkan(Device_Vulkan* device, Ref<Shader> computeShader_);
    ~PipeLine_Vulkan();
private:
    Device_Vulkan* device_;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    PipeLineLayout* layout_;
};

CONVERT_TO_VULKAN_INTERNAL(PipeLine)
}