#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi
{
DescriptorSetAllocator::DescriptorSetAllocator(Device_Vulkan* device, const DescriptorSetLayout& layout)
{
	QK_CORE_ASSERT(device != nullptr && !layout.bindings.empty())

	this->m_Device = device;
	this->m_layout = layout;
	// get pool size ratios
	for (auto& binding : layout.bindings) {
		auto& size_ratio = m_PoolSizeRatios.emplace_back();
		size_ratio.ratio = (float)binding.descriptorCount;
		size_ratio.type = binding.descriptorType;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo set_m_Layoutcreate_info = {};
	set_m_Layoutcreate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_m_Layoutcreate_info.pBindings = layout.bindings.data();
	set_m_Layoutcreate_info.bindingCount = (uint32_t)layout.bindings.size();
	VK_CHECK(vkCreateDescriptorSetLayout(device->vkDevice, &set_m_Layoutcreate_info, nullptr, &m_layout_handle))
}

void DescriptorSetAllocator::BeginFrame()
{
	m_SetNodes.BeginFrame();
}

DescriptorSetAllocator::~DescriptorSetAllocator()
{

	// destroy descriptor pools
	for (auto p : m_Pools)
		vkDestroyDescriptorPool(m_Device->vkDevice, p, nullptr);

	m_Pools.clear();

	// clear allocated nodes
	m_SetNodes.clear();

	vkDestroyDescriptorSetLayout(m_Device->vkDevice, m_layout_handle, nullptr);

	QK_CORE_LOGT_TAG("RHI", "Desctipor set allocator destroyed");
}

std::pair<VkDescriptorSet, bool> DescriptorSetAllocator::Find(size_t hash)
{
	auto* node = m_SetNodes.request(hash);
	if (node)
		return { node->set, true };

	node = m_SetNodes.request_vacant(hash);
	if (node)
		return { node->set, false };

	// need to create new descriptor pool and sets
	VkDescriptorPool pool;
	VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	info.maxSets = setsPerPool;

	std::vector<VkDescriptorPoolSize> poolSizes;
	if (!m_PoolSizeRatios.empty())
	{
		for (PoolSizeRatio ratio : m_PoolSizeRatios) 
		{
			poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * setsPerPool)
				});
		}
		info.poolSizeCount = (uint32_t)poolSizes.size();
		info.pPoolSizes = poolSizes.data();
	}

	// increment sets count per pool
	setsPerPool *= 2;

	if (vkCreateDescriptorPool(m_Device->vkDevice, &info, nullptr, &pool) != VK_SUCCESS)
	{
		QK_CORE_VERIFY(0, "Failed to create descriptor pool.");
		return { VK_NULL_HANDLE, false };
	}

	VkDescriptorSet sets[SET_BINDINGS_MAX_NUM];
	VkDescriptorSetLayout layouts[SET_BINDINGS_MAX_NUM];
	std::fill(std::begin(layouts), std::end(layouts), m_layout_handle);

	VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc.descriptorPool = pool;
	alloc.descriptorSetCount = SET_BINDINGS_MAX_NUM;
	alloc.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(m_Device->vkDevice, &alloc, sets) != VK_SUCCESS)
		QK_CORE_VERIFY(0, "Failed to allocate descriptor sets.");
	m_Pools.push_back(pool);

	for (auto set : sets)
		m_SetNodes.make_vacant(set);

	return { m_SetNodes.request_vacant(hash)->set, false };

}
}