#pragma once
#include "Quark/RHI/Shader.h"
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"

namespace quark::rhi {

struct ShaderResourceLayout {
    DescriptorSetLayout descriptorSetLayouts[DESCRIPTOR_SET_MAX_NUM] = {};
    VkPushConstantRange pushConstant = {};
    uint32_t descriptorSetLayoutMask = 0;
    uint32_t push_constant_hash = 0;
};

class Shader_Vulkan : public Shader {
public:
    Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize);
    ~Shader_Vulkan();

    const VkShaderModule GetShaderMoudule() const { return m_ShaderModule; }
    const VkPipelineShaderStageCreateInfo& GetStageInfo() const { return m_StageInfo; }
    const ShaderResourceLayout& GetResourceLayout() const { return m_ResourceLayout; }

private:
    Device_Vulkan* m_Device;
    VkShaderModule m_ShaderModule;
    VkPipelineShaderStageCreateInfo m_StageInfo;

    // Layout info
    ShaderResourceLayout m_ResourceLayout;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Shader)
}