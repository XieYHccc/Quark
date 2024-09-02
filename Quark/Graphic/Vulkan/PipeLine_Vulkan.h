    #pragma once
#include "Quark/Core/Util/TemporaryHashMap.h"
#include "Quark/Graphic/PipeLine.h"
#include "Quark/Graphic/RenderPassInfo.h"
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/Vulkan/Shader_Vulkan.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"

namespace quark::graphic {

// This struct is Cached in Device_Vulkan
struct PipeLineLayout {
    Device_Vulkan* device;
    VkPipelineLayout handle = VK_NULL_HANDLE; 

    DescriptorSetAllocator* setAllocators[DESCRIPTOR_SET_MAX_NUM] = {};
    VkDescriptorUpdateTemplate updateTemplate[DESCRIPTOR_SET_MAX_NUM] = {};
    ShaderResourceLayout combinedLayout = {};

    PipeLineLayout(Device_Vulkan* device, const ShaderResourceLayout combinedLayout);
    ~PipeLineLayout();
};

class PipeLine_Vulkan : public PipeLine {
public:
    PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc);
    PipeLine_Vulkan(Device_Vulkan* device, Ref<Shader> computeShader);
    ~PipeLine_Vulkan();

    VkPipeline GetHandle() const { return m_Handle; }
    const PipeLineLayout* GetLayout() const { return m_Layout; }
    const RenderPassInfo& GetCompatableRenderPassInfo() const { return m_CompatableRenderPassInfo; }
private:
    Device_Vulkan* m_Device;
    VkPipeline m_Handle = VK_NULL_HANDLE;
    PipeLineLayout* m_Layout = nullptr; // no lifetime management here
    RenderPassInfo m_CompatableRenderPassInfo;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(PipeLine)
}