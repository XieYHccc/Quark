#pragma once
#include "Quark/Core/Util/TemporaryHashMap.h"
#include "Quark/Graphic/PipeLine.h"
#include "Quark/Graphic/Shader.h"
#include "Quark/Graphic/RenderPassInfo.h"
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"

namespace quark::graphic {

// This struct is Cached in Device_Vulkan
struct PipeLineLayout {
    Device_Vulkan* device;
    VkPipelineLayout handle = VK_NULL_HANDLE; 
    // std::vector<VkImageViewType> imageViewTypes[DESCRIPTOR_SET_MAX_NUM];
    DescriptorSetLayout setLayouts[DESCRIPTOR_SET_MAX_NUM];
    DescriptorSetAllocator* setAllocators[DESCRIPTOR_SET_MAX_NUM];
    VkPushConstantRange pushConstant = {};
    u32 setLayoutMask = 0;
    VkDescriptorUpdateTemplate updateTemplate[DESCRIPTOR_SET_MAX_NUM];

    PipeLineLayout(Device_Vulkan* device, const std::array<DescriptorSetLayout, DESCRIPTOR_SET_MAX_NUM>& layouts, const VkPushConstantRange& push_constant, u32 layout_mask);
    ~PipeLineLayout();
};

class PipeLine_Vulkan : public PipeLine {
public:
    PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc);
    PipeLine_Vulkan(Device_Vulkan* device, Ref<Shader> computeShader_);
    ~PipeLine_Vulkan();

    VkPipeline GetHandle() const { return handle_; }
    const PipeLineLayout* GetLayout() const { return layout_; }
    const RenderPassInfo& GetRenderPassInfo() const { return renderPassInfo_; }
private:
    Device_Vulkan* device_;
    VkPipeline handle_ = VK_NULL_HANDLE;
    PipeLineLayout* layout_;
    RenderPassInfo renderPassInfo_;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(PipeLine)
}