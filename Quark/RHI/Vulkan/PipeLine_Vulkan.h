    #pragma once
#include "Quark/Core/Util/TemporaryHashMap.h"
#include "Quark/RHI/PipeLine.h"
#include "Quark/RHI/RenderPassInfo.h"
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Vulkan/Shader_Vulkan.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"

namespace quark::rhi {

// Once we have a list of VkDescriptorSetLayouts and push constant layouts, we now have our PipelineLayout.
// This is of course, hashed as well based on the hash of descriptor set layouts and push constant ranges.
struct PipeLineLayout {
    Device_Vulkan* device;
    VkPipelineLayout handle = VK_NULL_HANDLE; 

    DescriptorSetAllocator* setAllocators[DESCRIPTOR_SET_MAX_NUM] = {};
    VkDescriptorUpdateTemplate updateTemplate[DESCRIPTOR_SET_MAX_NUM] = {};
    CombinedResourceLayout combinedLayout = {};

    // shaders(vert shader, frag shader...) => combined resource layout => pipeline layout
    PipeLineLayout(Device_Vulkan* device, const CombinedResourceLayout& combinedLayout);
    ~PipeLineLayout();
};

class PipeLine_Vulkan : public PipeLine {
public:
    PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc);
    PipeLine_Vulkan(Device_Vulkan* device, Ref<Shader> computeShader);
    ~PipeLine_Vulkan();

    VkPipeline GetHandle() const { return m_handle; }
    const PipeLineLayout* GetLayout() const { return m_layout; }
    const GraphicPipeLineDesc& GetGraphicPipelineDesc() const { return m_graphicDesc; }

private:
    Device_Vulkan* m_device = nullptr;
    VkPipeline m_handle = VK_NULL_HANDLE;
    PipeLineLayout* m_layout = nullptr; // no lifetime management here
    GraphicPipeLineDesc m_graphicDesc = {};
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(PipeLine)
}