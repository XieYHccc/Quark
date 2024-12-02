#pragma once
#include "Quark/Core/Util/TemporaryHashMap.h"
#include "Quark/RHI/Vulkan/Common_Vulkan.h"

namespace quark::rhi {

struct DescriptorBinding {
    union {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
    uint32_t dynamicOffset = 0;
};

struct DescriptorSetLayout {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayoutBinding vk_bindings[SET_BINDINGS_MAX_NUM];

    // These masks here are mostly for debug purpose
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
// This class can be represented as:
//* The VkDescriptorSetLayout
//* A bunch of VkDescriptorPools which can only allocate VkDescriptorSets of this set layout. Pools are added on-demand.
//* A pool of unused VkDescriptorSets which are already allocated and can be freely updated.
//* A temporary hashmap which keeps track of which descriptor sets have been requested recently. 
//  This allows us to reuse descriptor sets directly. In the ideal case, we almost never actually need to call vkUpdateDescriptorSets. 
//  We end up with hash -> get VkDescriptorSet -> vkCmdBindDescriptorSets.
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

    void BeginFrame();
    VkDescriptorSetLayout GetLayout() const { return m_Layout; }
    std::pair<VkDescriptorSet, bool> Find(size_t hash);

private:
    Device_Vulkan* m_Device;

    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    std::vector<PoolSizeRatio> m_PoolSizeRatios;
    std::vector<VkDescriptorPool> m_Pools;

    uint32_t setsPerPool = DEFAULT_SETS_NUM_PER_POOL; // default value

    struct DescriptorSetNode : util::TemporaryHashmapEnabled<DescriptorSetNode>, util::IntrusiveListEnabled<DescriptorSetNode>
    {
        explicit DescriptorSetNode(VkDescriptorSet set_) : set(set_) {}
        VkDescriptorSet set;
    };
    util::TemporaryHashmap<DescriptorSetNode, DESCRIPTOR_SET_RING_SIZE, true> m_SetNodes;
};
}