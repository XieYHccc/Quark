#pragma once
#include "Core/Util/TemporaryHashMap.h"
#include "Graphic/Vulkan/Common_Vulkan.h"
#include "Graphic/PipeLine.h"
#include "Graphic/Shader.h"
#include "Graphic/RenderPassInfo.h"

namespace graphic {
struct DescriptorBinding {
    union{
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
    VkDeviceSize dynamicOffset = 0;
};

struct DescriptorSetLayout {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayoutBinding vk_bindings[SET_BINDINGS_MAX_NUM];

    // These masks are mostly for debug purpose
    u32 sampled_image_mask = 0;
	u32 storage_image_mask = 0;
	u32 uniform_buffer_mask = 0;
	u32 storage_buffer_mask = 0;
    u32 sampler_mask = 0;
	u32 separate_image_mask = 0;
    u32 input_attachment_mask = 0;
    u32 set_stage_mask = 0;
};

// This class is Cached in Device_Vulkan
class DescriptorSetAllocator {
public:
    static constexpr u32 DESCRIPTOR_SET_RING_SIZE = 8;
    static constexpr u32 DEFAULT_SETS_NUM_PER_POOL = 16;
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

    DescriptorSetAllocator(Device_Vulkan* device, const DescriptorSetLayout& layout);
    ~DescriptorSetAllocator();

    void begin_frame();
    VkDescriptorSetLayout get_layout() const { return layout_;}
    std::pair<VkDescriptorSet, bool> find(size_t hash);

private:
    Device_Vulkan* device_;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
    std::vector<PoolSizeRatio> poolSizeRatios_;
    std::vector<VkDescriptorPool> pools_;
    uint32_t setsPerPool_ = DEFAULT_SETS_NUM_PER_POOL; // default value

	struct DescriptorSetNode : util::TemporaryHashmapEnabled<DescriptorSetNode>, util::IntrusiveListEnabled<DescriptorSetNode>
	{
		explicit DescriptorSetNode(VkDescriptorSet set_) : set(set_) {}
		VkDescriptorSet set;
	};
    util::TemporaryHashmap<DescriptorSetNode, DESCRIPTOR_SET_RING_SIZE, true> set_nodes;
};

// This class is Cached in Device_Vulkan
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

CONVERT_TO_VULKAN_INTERNAL(PipeLine)
}