#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi
{
DescriptorSetAllocator::DescriptorSetAllocator(Device_Vulkan* device, const DescriptorSetLayout& layout)
{
	QK_CORE_ASSERT(device != nullptr && !layout.bindings.empty())

	this->m_device = device;
	this->m_layout = layout;
	// get pool size ratios
	for (auto& binding : layout.bindings) {
		auto& s = m_pool_sizes.emplace_back();
		s.type = binding.descriptorType;
		s.descriptorCount = binding.descriptorCount * s_num_sets_per_pool;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pBindings = layout.bindings.data();
	create_info.bindingCount = (uint32_t)layout.bindings.size();
	VK_CHECK(vkCreateDescriptorSetLayout(device->vkDevice, &create_info, nullptr, &m_layout_handle))
}

void DescriptorSetAllocator::BeginFrame()
{
	m_setNodes.BeginFrame();
}

DescriptorSetAllocator::~DescriptorSetAllocator()
{

	// destroy descriptor pools
	for (auto p : m_pools)
		vkDestroyDescriptorPool(m_device->vkDevice, p, nullptr);

	m_pools.clear();

	// clear allocated nodes
	m_setNodes.clear();

	vkDestroyDescriptorSetLayout(m_device->vkDevice, m_layout_handle, nullptr);

	QK_CORE_LOGT_TAG("RHI", "Desctipor set allocator destroyed");
}

std::pair<VkDescriptorSet, bool> DescriptorSetAllocator::RequestDescriptorSet(uint64_t hash)
{
	auto* node = m_setNodes.request(hash);
	if (node)
		return { node->set, true };

	node = m_setNodes.request_vacant(hash);
	if (node)
		return { node->set, false };

	// need to create new descriptor pool and sets
	VkDescriptorPool pool;
	VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	info.maxSets = s_num_sets_per_pool;
	info.poolSizeCount = (uint32_t)m_pool_sizes.size();
	info.pPoolSizes = m_pool_sizes.data();

	if (vkCreateDescriptorPool(m_device->vkDevice, &info, nullptr, &pool) != VK_SUCCESS)
	{
		QK_CORE_VERIFY(0, "Failed to create descriptor pool.");
		return { VK_NULL_HANDLE, false };
	}

	VkDescriptorSet sets[s_num_sets_per_pool];
	VkDescriptorSetLayout layouts[s_num_sets_per_pool];
	std::fill(std::begin(layouts), std::end(layouts), m_layout_handle);

	VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc.descriptorPool = pool;
	alloc.descriptorSetCount = s_num_sets_per_pool;
	alloc.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(m_device->vkDevice, &alloc, sets) != VK_SUCCESS)
		QK_CORE_VERIFY(0, "Failed to allocate descriptor sets.");
	m_pools.push_back(pool);

	for (auto set : sets)
		m_setNodes.make_vacant(set);

	return { m_setNodes.request_vacant(hash)->set, false };

}
}