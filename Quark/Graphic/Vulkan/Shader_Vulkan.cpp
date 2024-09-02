#include "Quark/qkpch.h"
#include "Quark/Graphic/Vulkan/Shader_Vulkan.h"
#include <spirv_reflect.h>
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"

namespace quark::graphic {
    
#define SPV_REFLECT_CHECK(x) CORE_ASSERT(x == SPV_REFLECT_RESULT_SUCCESS)

Shader_Vulkan::Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize)
    : Shader(stage), m_Device(device)
{
    CORE_DEBUG_ASSERT(m_Device != nullptr)
    CORE_LOGD("Creating vulkan shader...")

    VkDevice vk_device = m_Device->vkDevice;
    auto& vk_context = m_Device->vkContext;

    // Create shader module
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = codeSize;
    moduleInfo.pCode = (const uint32_t*)shaderCode;
    if (vkCreateShaderModule(vk_device, &moduleInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
        CORE_LOGE("Failed to create vulkan shader module.")


    // Fill shader stage info
    m_StageInfo = {};
    m_StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_StageInfo.module = m_ShaderModule;
    m_StageInfo.pName = "main";
    m_StageInfo.pNext = nullptr;
    m_StageInfo.flags = 0;
    switch (stage) 
    {
    case ShaderStage::STAGE_COMPUTE:
        m_StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    case ShaderStage::STAGE_VERTEX:
        m_StageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderStage::STAGE_FRAGEMNT:
        m_StageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    default:
    {
        CORE_DEBUG_ASSERT("ShaderStage not handled!")
        m_StageInfo.stage = VK_SHADER_STAGE_ALL;
        break;
    }
    }

    // Create descriptor set layout from reflection
    SpvReflectShaderModule spv_reflcet_module;
    auto result = spvReflectCreateShaderModule(moduleInfo.codeSize, moduleInfo.pCode, &spv_reflcet_module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        CORE_LOGE("Failed to reflect spv shader moudule")
    }
    uint32_t binding_count = 0;
    SPV_REFLECT_CHECK(spvReflectEnumerateDescriptorBindings(&spv_reflcet_module, &binding_count, nullptr))
    std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
    SPV_REFLECT_CHECK(spvReflectEnumerateDescriptorBindings(&spv_reflcet_module, &binding_count, bindings.data()))

    uint32_t push_constant_count = 0;
    SPV_REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&spv_reflcet_module, &push_constant_count, nullptr))
    std::vector<SpvReflectBlockVariable*> push_constants(push_constant_count);
    SPV_REFLECT_CHECK(spvReflectEnumeratePushConstantBlocks(&spv_reflcet_module, &push_constant_count, push_constants.data()))

    // Push constants
    for (auto& x : push_constants)
    {
        CORE_ASSERT(x->size < PUSH_CONSTANT_DATA_SIZE)
        m_ResourceLayout.pushConstant.stageFlags = m_StageInfo.stage;
        m_ResourceLayout.pushConstant.offset = std::min(m_ResourceLayout.pushConstant.offset, x->offset);
        m_ResourceLayout.pushConstant.size = std::max(m_ResourceLayout.pushConstant.size, x->size);
    }

    for (auto& b : bindings) {
        CORE_DEBUG_ASSERT(b->set < DESCRIPTOR_SET_MAX_NUM) // only support shaders with 4 sets or less

        uint32_t bind_slot = b->binding;
        uint32_t set = b->set; 

        m_ResourceLayout.descriptorSetLayoutMask |= 1 << set;

        VkDescriptorSetLayoutBinding& layout_binding = m_ResourceLayout.descriptorSetLayouts[set].bindings.emplace_back();
        layout_binding.binding = bind_slot;
        layout_binding.stageFlags = m_StageInfo.stage;
        layout_binding.descriptorCount = b->count;
        layout_binding.descriptorType = (VkDescriptorType)b->descriptor_type;

        if (layout_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            // For now, always replace VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER with VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
            // It would be quite messy to track which buffer is dynamic and which is not in the binding code, consider multiple pipeline bind points too
            // But maybe the dynamic uniform buffer is not always best because it occupies more registers (like DX12 root descriptor)?
            layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }
        
        // VkImageViewType& view_type = bindingViews_[set].emplace_back();
        // view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        // switch (binding->descriptor_type) {
        // default:
        //     break;
        // case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        // case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        //     switch (binding->image.dim) {
        //     default:
        //         CORE_DEBUG_ASSERT("only support 2D, 3D and cube image!")
        //         break;
        //     case SpvDim2D:
        //         if (binding->image.arrayed == 0)
        //             view_type = VK_IMAGE_VIEW_TYPE_2D;
        //         else
        //             view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        //         break;
        //     case SpvDim3D:
        //         view_type = VK_IMAGE_VIEW_TYPE_3D;
        //         break;
        //     case SpvDimCube:
        //         if (binding->image.arrayed == 0)
        //             view_type = VK_IMAGE_VIEW_TYPE_CUBE;
        //         else
        //             view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        //         break;
        //     }
        //     break;
        // }
    }
    spvReflectDestroyShaderModule(&spv_reflcet_module);

}

Shader_Vulkan::~Shader_Vulkan()
{
    auto& frame = m_Device->GetCurrentFrame();
    
    if (m_ShaderModule != VK_NULL_HANDLE) {
        frame.garbageShaderModules.push_back(m_ShaderModule);
    }
    CORE_LOGD("Vulkan shader destroyed")
}

}