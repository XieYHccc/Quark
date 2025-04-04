#pragma once
#include "Quark/Core/Util/TemporaryHashMap.h"
#include "Quark/RHI/Vulkan/Common_Vulkan.h"

namespace quark::rhi {

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
    static constexpr uint32_t s_ring_size = 8;
    static constexpr uint32_t s_num_sets_per_pool = 64;
    DescriptorSetAllocator(Device_Vulkan* device, const DescriptorSetLayout& layout);
    ~DescriptorSetAllocator();

    void BeginFrame();
    VkDescriptorSetLayout GetLayout() const { return m_layout_handle; }
    std::pair<VkDescriptorSet, bool> RequestDescriptorSet(uint64_t hash);

private:
    Device_Vulkan* m_device;
    VkDescriptorSetLayout m_layout_handle = VK_NULL_HANDLE;
    DescriptorSetLayout m_layout;
    std::vector<VkDescriptorPoolSize> m_pool_sizes;
    std::vector<VkDescriptorPool> m_pools;

    struct DescriptorSetNode : util::TemporaryHashmapEnabled<DescriptorSetNode>, util::IntrusiveListEnabled<DescriptorSetNode>
    {
        explicit DescriptorSetNode(VkDescriptorSet set_) : set(set_) {}
        VkDescriptorSet set;
    };
    util::TemporaryHashmap<DescriptorSetNode, s_ring_size, true> m_setNodes;
};
}