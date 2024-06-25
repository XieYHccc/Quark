#include "pch.h"
#include "Graphic/Vulkan/Shader_Vulkan.h"
#include <spirv_reflect.h>
#include "Graphic/Vulkan/Device_Vulkan.h"

namespace graphic {
    
#define SPV_REFLECT_CHECK(x) CORE_ASSERT(x == SPV_REFLECT_RESULT_SUCCESS)

Shader_Vulkan::Shader_Vulkan(Device_Vulkan* device, ShaderStage stage, const void* shaderCode, size_t codeSize)
    : Shader(stage), device_(device)
{
    CORE_DEBUG_ASSERT(device_ != nullptr)
    CORE_LOGD("Creating vulkan shader...")

    VkDevice vk_device = device_->vkDevice;
    auto& vk_context = device_->context;

    // Create shader module
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = codeSize;
    moduleInfo.pCode = (const uint32_t*)shaderCode;
    if (vkCreateShaderModule(vk_device, &moduleInfo, nullptr, &shaderModule_) != VK_SUCCESS)
        CORE_LOGE("Failed to create vulkan shader module.")
    stageInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo_.module = shaderModule_;
    stageInfo_.pName = "main";
    switch (stage) 
    {
    case ShaderStage::STAGE_COMPUTE:
        stageInfo_.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    case ShaderStage::STAGE_VERTEX:
        stageInfo_.stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderStage::STAGE_FRAGEMNT:
        stageInfo_.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    default:
    {
        CORE_DEBUG_ASSERT("ShaderStage not handled!")
        stageInfo_.stage = VK_SHADER_STAGE_ALL;
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

    for (auto& x : push_constants)
    {
        CORE_ASSERT(x->size < PUSH_CONSTANT_DATA_SIZE)
        pushConstant_.stageFlags = stageInfo_.stage;
        pushConstant_.offset = x->offset;
        pushConstant_.size = x->size;
    }

    for (auto& binding : bindings) {
        CORE_DEBUG_ASSERT(binding->set < DESCRIPTOR_SET_MAX_NUM) // only support shaders with 4 sets or less

        auto bind_slot = binding->binding;
        auto set = binding->set; 

        auto& descriptor_binding = bindings_[set].emplace_back();
        CORE_ASSERT(binding->count == 1) // limit this to 1 for the simplity of updating VkDescriptor set
        descriptor_binding.binding = bind_slot;
        descriptor_binding.stageFlags = stageInfo_.stage;
        descriptor_binding.descriptorCount = binding->count;
        descriptor_binding.descriptorType = (VkDescriptorType)binding->descriptor_type;

        if (descriptor_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            // For now, always replace VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER with VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
            // It would be quite messy to track which buffer is dynamic and which is not in the binding code, consider multiple pipeline bind points too
            // But maybe the dynamic uniform buffer is not always best because it occupies more registers (like DX12 root descriptor)?
            descriptor_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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
    auto& frame = device_->GetCurrentFrame();
    
    if (shaderModule_ != VK_NULL_HANDLE) {
        frame.garbageShaderModules.push_back(shaderModule_);
    }
    CORE_LOGD("Shader module destroyed")
}

}