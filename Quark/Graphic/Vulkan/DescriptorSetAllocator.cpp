#include "Quark/qkpch.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"

namespace quark::graphic
{
DescriptorSetAllocator::DescriptorSetAllocator(Device_Vulkan* device, const DescriptorSetLayout& layout)
{
	CORE_DEBUG_ASSERT(device != nullptr && !layout.bindings.empty())

		this->device_ = device;

	// get pool size ratios
	for (auto& binding : layout.bindings) {
		auto& size_ratio = poolSizeRatios_.emplace_back();
		size_ratio.ratio = binding.descriptorCount;
		size_ratio.type = binding.descriptorType;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo set_layout_create_info = {};
	set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_create_info.pBindings = layout.bindings.data();
	set_layout_create_info.bindingCount = layout.bindings.size();
	VK_CHECK(vkCreateDescriptorSetLayout(device->vkDevice, &set_layout_create_info, nullptr, &layout_))
}

void DescriptorSetAllocator::BeginFrame()
{
	set_nodes.BeginFrame();
}
DescriptorSetAllocator::~DescriptorSetAllocator()
{

	// destroy descriptor pools
	for (auto p : pools_) {
		vkDestroyDescriptorPool(device_->vkDevice, p, nullptr);
	}
	pools_.clear();

	// clear allocated nodes
	set_nodes.clear();

	vkDestroyDescriptorSetLayout(device_->vkDevice, layout_, nullptr);

	CORE_LOGD("Desctipor set allocator destroyed")
}

std::pair<VkDescriptorSet, bool> DescriptorSetAllocator::Find(size_t hash)
{
	auto* node = set_nodes.request(hash);
	if (node)
		return { node->set, true };

	node = set_nodes.request_vacant(hash);
	if (node)
		return { node->set, false };

	// need to create new descriptor pool and sets
	VkDescriptorPool pool;
	VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	info.maxSets = setsPerPool_;

	std::vector<VkDescriptorPoolSize> poolSizes;
	if (!poolSizeRatios_.empty())
	{
		for (PoolSizeRatio ratio : poolSizeRatios_) {
			poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * setsPerPool_)
				});
		}
		info.poolSizeCount = poolSizes.size();
		info.pPoolSizes = poolSizes.data();
	}

	// increment sets count per pool
	setsPerPool_ *= 2;

	if (vkCreateDescriptorPool(device_->vkDevice, &info, nullptr, &pool) != VK_SUCCESS)
	{
		CORE_LOGE("Failed to create descriptor pool.");
		return { VK_NULL_HANDLE, false };
	}

	VkDescriptorSet sets[SET_BINDINGS_MAX_NUM];
	VkDescriptorSetLayout layouts[SET_BINDINGS_MAX_NUM];
	std::fill(std::begin(layouts), std::end(layouts), layout_);

	VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc.descriptorPool = pool;
	alloc.descriptorSetCount = SET_BINDINGS_MAX_NUM;
	alloc.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device_->vkDevice, &alloc, sets) != VK_SUCCESS)
		CORE_LOGE("Failed to allocate descriptor sets.");
	pools_.push_back(pool);

	for (auto set : sets)
		set_nodes.make_vacant(set);

	return { set_nodes.request_vacant(hash)->set, false };

}
}