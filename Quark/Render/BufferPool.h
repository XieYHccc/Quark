#pragma once
#include "Quark/RHI/Buffer.h"

namespace quark 
{
struct BufferBlockAllocation
{
	uint8_t* host;
	Ref<rhi::Buffer> buffer;
	size_t offset;
	size_t padded_size;
};

class BufferBlock
{
public:
	~BufferBlock() = default;

	BufferBlockAllocation Allocate(size_t allocate_size);

	size_t GetSize() const { return m_size; }
	size_t GetOffset() const { return m_offset; }

private:
	BufferBlock() = default;

	Ref<rhi::Buffer> m_buffer;
	size_t m_offset = 0;
	size_t m_size = 0;
	size_t m_alignment = 0;
	size_t m_spill_size = 0;
		
	friend class BufferPool;
};

class BufferPool
{
public:
	BufferPool(rhi::Device* device, size_t block_size, size_t alignment, uint32_t buffer_usage_bits);
	~BufferPool() = default;
		
	// Used for allocating UBOs, where we want to specify a fixed size for range,
	// and we need to make sure we don't allocate beyond the block.
	void SetSpillSize(size_t spill_size);

	size_t GetBlockSize() const { return m_block_size; }

	BufferBlock RequestBlock(size_t minimum_size);
	void RecycleBlock(BufferBlock block);

private:
	BufferBlock AllocateBlock(size_t size);

	rhi::Device* m_device;
	size_t m_block_size;
	size_t m_alignment;
	uint32_t m_buffer_usage_bits;
	std::vector<BufferBlock> m_blocks;
};
}