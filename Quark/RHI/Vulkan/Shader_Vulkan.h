#pragma once
#include "Quark/RHI/Shader.h"
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"

namespace quark::rhi {

struct ShaderResourceLayout {
    DescriptorSetLayout descriptor_set_layouts[DESCRIPTOR_SET_MAX_NUM] = {};
    VkPushConstantRange push_constant_range = {};
    uint32_t descriptor_set_mask = 0;
    uint32_t input_mask = 0;
    uint32_t output_mask = 0;
};

struct CombinedResourceLayout
{
    uint32_t attribute_mask = 0;
    uint32_t render_target_mask = 0;
    DescriptorSetLayout descriptor_set_layouts[DESCRIPTOR_SET_MAX_NUM] = {};
    // uint32_t stages_for_bindings[DESCRIPTOR_SET_MAX_NUM][SET_BINDINGS_MAX_NUM] = {};
    uint32_t stages_for_sets[DESCRIPTOR_SET_MAX_NUM] = {};
    VkPushConstantRange push_constant_range = {};
    uint32_t descriptor_set_mask = 0;
    util::Hash push_constant_hash = 0;
};

class Shader_Vulkan : public Shader {
public:
    Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize);
    ~Shader_Vulkan();

    const VkShaderModule GetShaderMoudule() const { return m_shaderModule; }
    const VkPipelineShaderStageCreateInfo& GetStageInfo() const { return m_stageInfo; }
    const ShaderResourceLayout& GetResourceLayout() const { return m_resourceLayout; }

private:
    Device_Vulkan* m_device;
    VkShaderModule m_shaderModule;
    VkPipelineShaderStageCreateInfo m_stageInfo;

    // Layout info
    ShaderResourceLayout m_resourceLayout;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Shader)
}