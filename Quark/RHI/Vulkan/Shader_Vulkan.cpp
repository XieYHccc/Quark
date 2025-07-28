#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/Shader_Vulkan.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/Core/Util/Hash.h"

#include <spirv_reflect.h>

#define SPV_REFLECT_CHECK(x) QK_CORE_VERIFY(x == SPV_REFLECT_RESULT_SUCCESS)

namespace quark::rhi {
   
Shader_Vulkan::Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize)
    : Shader(stage), m_device(device)
{
    QK_CORE_ASSERT(m_device != nullptr)

    VkDevice vk_device = m_device->vkDevice;
    auto& vk_context = m_device->GetVulkanContext();

    // Create shader module
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = codeSize;
    moduleInfo.pCode = (const uint32_t*)shaderCode;
    if (vkCreateShaderModule(vk_device, &moduleInfo, nullptr, &m_shaderModule) != VK_SUCCESS)
        QK_CORE_LOGE_TAG("RHI", "Failed to create vulkan shader module.");


    // Fill shader stage info
    m_stageInfo = {};
    m_stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_stageInfo.module = m_shaderModule;
    m_stageInfo.pName = "main";
    m_stageInfo.pNext = nullptr;
    m_stageInfo.flags = 0;
    switch (stage) 
    {
    case ShaderStage::STAGE_COMPUTE:
        m_stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    case ShaderStage::STAGE_VERTEX:
        m_stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderStage::STAGE_FRAGEMNT:
        m_stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    default:
    {
        QK_CORE_ASSERT("ShaderStage not handled!")
        m_stageInfo.stage = VK_SHADER_STAGE_ALL;
        break;
    }
    }

    // Create descriptor set layout from reflection
    SpvReflectShaderModule spv_reflcet_module;
    auto result = spvReflectCreateShaderModule(moduleInfo.codeSize, moduleInfo.pCode, &spv_reflcet_module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        QK_CORE_LOGE_TAG("RHI", "Failed to reflect spv shader moudule");

    uint32_t binding_count = 0;
    SPV_REFLECT_CHECK(spvReflectEnumerateDescriptorBindings(&spv_reflcet_module, &binding_count, nullptr));
    std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
    SPV_REFLECT_CHECK(spvReflectEnumerateDescriptorBindings(&spv_reflcet_module, &binding_count, bindings.data()));

    uint32_t push_constant_count = 0;
    SPV_REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&spv_reflcet_module, &push_constant_count, nullptr));
    std::vector<SpvReflectBlockVariable*> push_constants(push_constant_count);
    SPV_REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&spv_reflcet_module, &push_constant_count, push_constants.data()));

    uint32_t input_count = 0;
    SPV_REFLECT_CHECK(spvReflectEnumerateInputVariables(&spv_reflcet_module, &input_count, nullptr));
    std::vector<SpvReflectInterfaceVariable*> inputs(input_count);
    SPV_REFLECT_CHECK(spvReflectEnumerateInputVariables(&spv_reflcet_module, &input_count, inputs.data()));

    uint32_t output_count = 0;
    SPV_REFLECT_CHECK(spvReflectEnumerateOutputVariables(&spv_reflcet_module, &output_count, nullptr));
    std::vector<SpvReflectInterfaceVariable*> outputs(output_count);
    SPV_REFLECT_CHECK(spvReflectEnumerateOutputVariables(&spv_reflcet_module, &output_count, outputs.data()));

    // Inputs
    for (uint32_t i = 0; i < input_count; ++i) {
        SpvReflectInterfaceVariable* var = inputs[i];
        if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
            continue; // skip built-in variables like gl_Position
        if (var->location < 32) // only support first 32 inputs
            m_resourceLayout.input_mask |= (1u << var->location);
    }

    // Outputs
    for (uint32_t i = 0; i < output_count; ++i) {
        SpvReflectInterfaceVariable* var = outputs[i];
        if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
            continue; // skip built-in variables like gl_FragDepth
        if (var->location < 32) // only support first 32 outputs
            m_resourceLayout.output_mask |= (1u << var->location);
    }

    // Push constants
    for (auto& x : push_constants)
    {
        QK_CORE_ASSERT(x->size < PUSH_CONSTANT_DATA_SIZE)
        m_resourceLayout.push_constant_range.stageFlags = m_stageInfo.stage;
        m_resourceLayout.push_constant_range.offset = std::min(m_resourceLayout.push_constant_range.offset, x->offset);
        m_resourceLayout.push_constant_range.size = std::max(m_resourceLayout.push_constant_range.size, x->size);
    }

    for (auto& b : bindings) {
        QK_CORE_ASSERT(b->set < DESCRIPTOR_SET_MAX_NUM) // only support shaders with 4 sets or less

        uint32_t bind_slot = b->binding;
        uint32_t set = b->set; 

        m_resourceLayout.descriptor_set_mask |= 1 << set;
        // m_resourceLayout.descriptor_set_layouts[set].set_stage_mask |= m_stageInfo.stage;

        VkDescriptorSetLayoutBinding& layout_binding = m_resourceLayout.descriptor_set_layouts[set].bindings.emplace_back();
        layout_binding.binding = bind_slot;
        layout_binding.stageFlags = m_stageInfo.stage;
        layout_binding.descriptorCount = b->count;
        layout_binding.descriptorType = (VkDescriptorType)b->descriptor_type;

        if (layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            // For now, always replace VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER with VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
            // It would be quite messy to track which buffer is dynamic and which is not in the binding code, consider multiple pipeline bind points too
            // But maybe the dynamic uniform buffer is not always best because it occupies more registers (like DX12 root descriptor)?
            layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }

        DescriptorSetLayout& set_layout = m_resourceLayout.descriptor_set_layouts[set];
        switch (layout_binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            set_layout.uniform_buffer_mask |= 1u << bind_slot;
            break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            set_layout.storage_buffer_mask |= 1u << bind_slot;
            break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            set_layout.sampled_image_mask |= 1u << bind_slot;
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            set_layout.separate_image_mask |= 1u << bind_slot;
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            set_layout.sampler_mask |= 1u << bind_slot;
            break;
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			set_layout.input_attachment_mask |= 1u << bind_slot;
			break;
        default:
            QK_CORE_VERIFY("Descriptor type not handled!");
            break;
        }
        
        
    }
    spvReflectDestroyShaderModule(&spv_reflcet_module);

}

Shader_Vulkan::~Shader_Vulkan()
{
    auto& frame = m_device->GetCurrentFrame();
    
    if (m_shaderModule != VK_NULL_HANDLE) {
        frame.garbage_shaderModules.push_back(m_shaderModule);
    }
}

}