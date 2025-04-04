#pragma once
#include "Quark/Core/Base.h"

#include <vulkan/vulkan.h>

namespace quark::rhi 
{
class Buffer;
class Device_Vulkan;

struct BufferBlockAllocation
{
	uint8_t* host;
	Ref<Buffer> buffer;
	VkDeviceSize offset;
	VkDeviceSize padded_size;
};

class BufferBlock
{
public:
	~BufferBlock() = default;
	BufferBlock() = default;
	BufferBlockAllocation Allocate(VkDeviceSize allocate_size);

	inline VkDeviceSize GetSize() const { return m_size; }
	inline VkDeviceSize GetOffset() const { return m_offset; }

private:
	Ref<Buffer> m_buffer;
	VkDeviceSize m_offset = 0;
	VkDeviceSize m_size = 0;
	VkDeviceSize m_alignment = 0;
	VkDeviceSize m_spill_size = 0;
	uint8_t* m_mapped = nullptr;

	friend class BufferPool;
};

class BufferPool
{
public:
	~BufferPool() = default;

	void Init(Device_Vulkan* device, VkDeviceSize block_size, VkDeviceSize alignment, VkBufferUsageFlags buffer_usage_bits);
	void Reset();

	// used for allocating UBOs, where we want to specify a fixed size for range,
	// and we need to make sure we don't allocate beyond the block.
	void SetSpillRegionSize(VkDeviceSize spill_size);
	void SetMaxRetainedBlocks(size_t max_blocks);

	VkDeviceSize GetBlockSize() const { return m_block_size; }

	BufferBlock RequestBlock(VkDeviceSize minimum_size);
	void RecycleBlock(BufferBlock block);

private:
	BufferBlock AllocateBlock(VkDeviceSize size);

	Device_Vulkan* m_device;
	VkDeviceSize m_block_size;
	VkDeviceSize m_spill_size;
	VkDeviceSize m_alignment;
	VkBufferUsageFlags m_buffer_usage_bits;
	size_t m_max_retained_blocks = 0;
	std::vector<BufferBlock> m_blocks;
};

}