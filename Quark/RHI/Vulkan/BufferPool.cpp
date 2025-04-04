#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/BufferPool.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi
{

void BufferPool::Init(Device_Vulkan* device, VkDeviceSize block_size, VkDeviceSize alignment, VkBufferUsageFlags buffer_usage_bits)
{
	m_device = device;
	m_block_size = block_size;
	m_alignment = alignment;
	m_buffer_usage_bits = buffer_usage_bits;
}

void BufferPool::Reset()
{
	m_blocks.clear();
}

void BufferPool::SetSpillRegionSize(VkDeviceSize spill_size)
{
	m_spill_size = spill_size;
}

void BufferPool::SetMaxRetainedBlocks(size_t max_blocks)
{
	m_max_retained_blocks = max_blocks;
}

BufferBlock BufferPool::AllocateBlock(VkDeviceSize size)
{
	BufferBlock block;

	BufferDesc desc;
	desc.size = size;
	desc.usageBits = m_buffer_usage_bits;
	desc.domain = BufferMemoryDomain::CPU;

	block.m_buffer = m_device->CreateBuffer(desc, nullptr);
	auto& internal = ToInternal(block.m_buffer.get());
	internal.SetInternalSynced();
	m_device->SetDebugName(block.m_buffer, "chain-allocated-block");
	// host visible buffer can be mapped
	block.m_mapped = static_cast<uint8_t*>(block.m_buffer->GetMappedDataPtr());
	block.m_offset = 0;
	block.m_size = size;
	block.m_alignment = m_alignment;
	block.m_spill_size = m_spill_size;

	return block;

}

BufferBlock BufferPool::RequestBlock(VkDeviceSize minimum_size)
{
	if (minimum_size > m_block_size || m_blocks.empty())
	{
		return AllocateBlock(std::max(minimum_size, m_block_size));
	}
	else
	{
		auto back = m_blocks.back();
		m_blocks.pop_back();
		back.m_offset = 0;
		return back;
	}
}

void BufferPool::RecycleBlock(BufferBlock block)
{
	QK_CORE_ASSERT(block.m_size == m_block_size);
	if (m_blocks.size() < m_max_retained_blocks)
	{
		m_blocks.push_back(block);
	}
	else
	{
		block = {};
	}
}

BufferBlockAllocation BufferBlock::Allocate(VkDeviceSize allocate_size)
{
	VkDeviceSize aligned_offset = (m_offset + m_alignment - 1) & ~(m_alignment - 1);

	if (aligned_offset + allocate_size <= m_size)
	{
		uint8_t* ret = m_mapped + aligned_offset;
		m_offset = aligned_offset + allocate_size;

		VkDeviceSize padded_size = std::max<VkDeviceSize>(allocate_size, m_spill_size);
		padded_size = std::min<VkDeviceSize>(padded_size, m_size - aligned_offset);
		return { ret, m_buffer, aligned_offset, padded_size };
	}
	else
	{
		return { nullptr, {}, 0, 0 };
	}
}

}