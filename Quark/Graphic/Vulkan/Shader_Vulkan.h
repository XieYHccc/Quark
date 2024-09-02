#pragma once
#include "Quark/Graphic/Shader.h"
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"

namespace quark::graphic {

struct ShaderResourceLayout {
    DescriptorSetLayout descriptorSetLayouts[DESCRIPTOR_SET_MAX_NUM] = {};
    VkPushConstantRange pushConstant = {};
    uint32_t descriptorSetLayoutMask = 0;
};

class Shader_Vulkan : public Shader {
public:
    Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize);
    ~Shader_Vulkan();

    const VkShaderModule GetShaderMoudule() const { return m_ShaderModule; }
    const VkPipelineShaderStageCreateInfo& GetStageInfo() const { return m_StageInfo; }
    const std::vector<VkDescriptorSetLayoutBinding>& GetBindings(u32 set) const { return bindings_[set]; }
    const VkPushConstantRange& GetPushConstant() const { return pushConstant_; }
    const ShaderResourceLayout& GetResourceLayout() const { return m_ResourceLayout; }
private:
    Device_Vulkan* m_Device;
    VkShaderModule m_ShaderModule;
    VkPipelineShaderStageCreateInfo m_StageInfo;

    // Layout info
    std::vector<VkDescriptorSetLayoutBinding> bindings_[DESCRIPTOR_SET_MAX_NUM];
    VkPushConstantRange pushConstant_;
    ShaderResourceLayout m_ResourceLayout;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Shader)
}