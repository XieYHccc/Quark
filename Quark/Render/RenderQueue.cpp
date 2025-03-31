#include "Quark/qkpch.h"
#include "Quark/Render/RenderQueue.h"
#include "Quark/Render/IRenderable.h"
#include "Quark/Render/RenderContext.h"
#include "Quark/Core/Math/Util.h"

namespace quark
{
RenderQueue::~RenderQueue()
{
	RecycleBlocks();
}

void* RenderQueue::Allocate(size_t size, size_t alignment)
{
	if (size + alignment > Block::block_size)
	{
		Block* block = InsertLargeBlock(size, alignment);
		return AllocateFromBlock(*block, size, alignment);
	}

	// first allocation
	if (!m_cur_block)
		m_cur_block = InsertBlock();

	void* data = AllocateFromBlock(*m_cur_block, size, alignment);
	if (data)
		return data;

	m_cur_block = InsertBlock();
	data = AllocateFromBlock(*m_cur_block, size, alignment);
	return data;
}

const RenderQueue::RenderQueueTaskVector RenderQueue::GetQueueTasks(Queue queue_type) const
{
	return m_queues[util::ecast(queue_type)];
}

void RenderQueue::PushRenderables(const RenderContext& context, const RenderableInfo* renderables, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		renderables[i].renderable->GetRenderData(context, renderables[i].render_info, *this);
	}
}

void RenderQueue::Reset()
{
	RecycleBlocks();
	for (auto& q : m_queues)
	{
		q.clear();
	}
	m_perdrawcall_data.clear();
}

void RenderQueue::Sort()
{
	for (auto& q : m_queues)
	{
		q.util_indices.resize(q.raw_input.size());
		q.sorted_output.resize(q.raw_input.size());
		std::iota(q.util_indices.begin(), q.util_indices.end(), 0);
		std::sort(q.util_indices.begin(), q.util_indices.end(), [&](const uint32_t& iA, const uint32_t& iB)
		{
				return q.raw_input[iA].sorting_key < q.raw_input[iB].sorting_key;
		});

		for (size_t i = 0; i < q.raw_input.size(); i++)
		{
			q.sorted_output[i] = q.raw_input[q.util_indices[i]];
		}
	}
}

void RenderQueue::Dispatch(Queue que, rhi::CommandList& cmd) const
{
	const RenderQueueTaskVector& queue = m_queues[util::ecast(que)];

	// assert that we did in fact sort.
	QK_CORE_ASSERT(m_queues[util::ecast(que)].sorted_output.size() == m_queues[util::ecast(que)].raw_input.size());

	const RenderQueueTask* tasks = queue.sorted_output.data();
	size_t begin = 0, end = queue.sorted_output.size();
	while (begin < end)
	{
		uint8_t instances = 1;
		for (size_t i = begin + 1; i < end && tasks[i].perdrawcall_data == tasks[begin].perdrawcall_data; i++)
		{
			QK_CORE_ASSERT(tasks[i].render == tasks[begin].render);
			instances++;
		}

		tasks[begin].render(cmd, &tasks[begin], instances);
		begin += instances;
	}
}

RenderQueue::Block* RenderQueue::InsertBlock()
{
	Block* ret = m_block_pool.allocate();
	m_blocks.push_back(ret);
	return ret;
}

RenderQueue::Block* RenderQueue::InsertLargeBlock(size_t size, size_t alignment)
{
	size_t padded_size = alignment > alignof(uintmax_t) ? (size + alignment) : size;
	Block* ret = m_block_pool.allocate(padded_size);
	m_blocks.push_back(ret);
	return ret;
}

void* RenderQueue::AllocateFromBlock(Block& block, size_t size, size_t alignment)
{
	block.ptr = (block.ptr + alignment - 1) & ~(alignment - 1);
	uintptr_t end = block.ptr + size;
	if (end <= block.end)
	{
		void* ret = reinterpret_cast<void*>(block.ptr);
		block.ptr = end;
		return ret;
	}
	else
		return nullptr;
}
void RenderQueue::RecycleBlocks()
{
	for (Block* block : m_blocks)
	{
		m_block_pool.free(block);
	}
	
	m_blocks.clear();
	m_cur_block = nullptr;
}


uint64_t RenderInfo::GetSortKey(const RenderContext& context, Queue queue_type, util::Hash pipeline_hash, util::Hash draw_hash, const glm::vec3& center, StaticLayer layer)
{
	const CameraParameters& camera = context.GetCameraParameters();
	float z = glm::dot(center - camera.camera_position, camera.camera_front);
	return GetSpriteSortKey(queue_type, pipeline_hash, draw_hash, z, layer);
}

uint64_t RenderInfo::GetSpriteSortKey(Queue queue_type, util::Hash pipeline_hash, util::Hash draw_hash, float depth, StaticLayer layer)
{
	// Monotonically increasing floating point will be monotonic in uint32_t as well when z is non - negative.
	depth = std::max(0.f, depth);
	uint32_t depth_key = math::FloatBitsToUint32(depth);

	pipeline_hash &= 0xffff0000u;
	pipeline_hash |= draw_hash & 0xffffu;

	if (queue_type == Queue::Transparent)
	{
		depth_key ^= 0xffffffffu; // Back-to-front instead.
		// Prioritize correct back-to-front rendering over pipeline.
		return (uint64_t(depth_key) << 32) | pipeline_hash;
	}
	else
	{
#if 1
		// Prioritize state changes over depth.
		depth_key >>= 2;
		return (uint64_t(util::ecast(layer)) << 62) | (uint64_t(pipeline_hash) << 30) | depth_key;
#else
		// Prioritize front-back sorting over state changes.
		pipeline_hash >>= 2;
		return (uint64_t(ecast(layer)) << 62) | (uint64_t(depth_key) << 30) | pipeline_hash;
#endif
	}
}

}