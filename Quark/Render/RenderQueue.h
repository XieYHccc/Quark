#pragma once
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/Util/ObjectPool.h"

#include <glm/glm.hpp>

#include <unordered_map>

namespace quark 
{
namespace rhi{ class CommandList; }

class IRenderable;
class RenderContext;
struct RenderInfoCmpt;

struct RenderableInfo 
{
    IRenderable* renderable;
    RenderInfoCmpt* render_info;
    util::Hash transform_hash;
};
using VisibilityList = std::vector<RenderableInfo>;

enum class Queue : uint8_t
{
    Opaque,
    OpaqueEmissive,
    Light, // Relevant only for classic deferred rendering
	Transparent,
	Count
};

enum class StaticLayer : uint8_t
{
	Front,
	Default,
	Back,
	Last,
	Count
};

struct RenderInfo
{
    // deprecated!!!
    static uint64_t GetSortKey(const RenderContext& context, Queue queue_type,
                               util::Hash pipeline_hash, util::Hash draw_hash,
                               const glm::vec3& center,
                               StaticLayer layer = StaticLayer::Default);
    static uint64_t GetSortKey(const RenderContext& context, Queue queue_type,
                               util::Hash pipeline_hash, util::Hash material_hash,
                               util::Hash draw_hash, const glm::vec3& center,
                               StaticLayer layer = StaticLayer::Default);

    // deprecated!!
    static uint64_t GetSpriteSortKey(Queue queue_type,
                                     util::Hash pipeline_hash, util::Hash draw_hash,
                                     float depth, StaticLayer layer = StaticLayer::Default);
    static uint64_t GetSpriteSortKey(Queue queue_type,
                                     util::Hash pipeline_hash, util::Hash material_hash,
                                     util::Hash vbo_hash, float depth, 
                                     StaticLayer layer = StaticLayer::Default);
};

struct RenderQueueTask;
using RenderFunc = void (*)(rhi::CommandList&, const RenderQueueTask*, unsigned);

struct RenderQueueTask
{
    // how to render an object.
	RenderFunc render;

	// per-draw call specific data. Understood by the render callback.
	const void* perdrawcall_data;

	// per-instance specific data. Understood by the render callback.
	const void* instance_data;

	// sorting key.
	// lower sorting keys will appear earlier.
	uint64_t sorting_key;
};

struct PerDrawcallDataWrappedErased
{

};

template<typename T>
struct PerDrawcallDataWrapped : public PerDrawcallDataWrappedErased
{
    T data;
};

// Collect render tasks(draw calls) and sort them.
class RenderQueue 
{
public:
    struct RenderQueueTaskVector
    {
        static const size_t init_size = 64;
        std::vector<RenderQueueTask> raw_input;
        std::vector<RenderQueueTask> sorted_output;
        std::vector<uint32_t> util_indices; // TODO: remove

        void clear()
		{
			raw_input.clear();
			sorted_output.clear();
            util_indices.clear();
		}
    };

    RenderQueue() = default;
    void operator=(const RenderQueue &) = delete;
	RenderQueue(const RenderQueue &) = delete;
	~RenderQueue();

    template<typename T>
    T* PushTask(Queue queue_type, util::Hash instance_key, uint64_t sorting_key,
            RenderFunc render_func, void* instance_data)
    {
        QK_STATIC_ASSERT(std::is_trivially_destructible<T>::value, "Dispatchable type is not trivially destructible!");
        QK_CORE_ASSERT(instance_key != 0 && sorting_key != 0);

        util::Hasher h(instance_key);
        h.pointer(render_func);

        using WrappedT = PerDrawcallDataWrapped<T>;

        auto find = m_perdrawcall_data.find(h.get());
        if (find != m_perdrawcall_data.end())
        {
            WrappedT* wrapped_t = static_cast<WrappedT*>(find->second);
            m_queues[util::ecast(queue_type)].raw_input.push_back({ render_func, &wrapped_t->data, instance_data, sorting_key });
        }
		else
		{
			void* buffer = Allocate(sizeof(WrappedT), alignof(WrappedT));
            QK_CORE_VERIFY(buffer, "Failed to allocate memory for per-drawcall data");

            WrappedT* wrapped_t = new(buffer) WrappedT();
            m_perdrawcall_data[h.get()] = wrapped_t;
            m_queues[util::ecast(queue_type)].raw_input.push_back({ render_func, &wrapped_t->data, instance_data, sorting_key });

            return &wrapped_t->data;
		}

        return nullptr;
    }
    
    void* Allocate(size_t size, size_t alignment);

    template<typename T>
    T* AllocateOne()
    {
        QK_STATIC_ASSERT(std::is_trivially_destructible<T>::value, "Type is not trivially destructible!");
		auto* t = static_cast<T*>(Allocate(sizeof(T), alignof(T)));
		if (t)
			new (t) T();
		return t;
    }

	template <typename T>
	T* AllocateMany(size_t n)
	{
		QK_STATIC_ASSERT(std::is_trivially_destructible<T>::value, "Type is not trivially destructible!");
		auto* t = static_cast<T*>(Allocate(sizeof(T) * n, alignof(T)));
		if (t)
			for (size_t i = 0; i < n; i++)
				new (&t[i]) T();
		return t;
	}

    const RenderQueueTaskVector GetQueueTasks(Queue queue_type) const;
    const std::string& GetPassName() const { return m_pass_name; }

    void SetPassName(const std::string& name) { m_pass_name = name; }   // For renderables selecting shader program
    void PushRenderables(const RenderContext& context, const RenderableInfo* renderables, size_t count);
    void Reset();
    void Sort();
    void Dispatch(Queue que, rhi::CommandList& cmd) const;

private:
    struct Block 
    {
        static const uint32_t block_size = 64 * 1024;

        std::unique_ptr<uint8_t[]> large_buffer;
        uintptr_t begin;
        uintptr_t end;
        uintptr_t ptr;
        uint8_t inline_buffer[block_size];

        explicit Block(size_t size)
        {
            large_buffer.reset(new uint8_t[size]);
            begin = reinterpret_cast<uintptr_t>(large_buffer.get());
            end = reinterpret_cast<uintptr_t>(large_buffer.get() + size);
            reset();
        };

        Block()
        {
            begin = reinterpret_cast<uintptr_t>(inline_buffer);
			end = reinterpret_cast<uintptr_t>(inline_buffer) + block_size;
			reset();
        }

        void operator=(const Block&) = delete;
        Block(const Block&) = delete;

        void reset() { ptr = begin; }
    };

    Block* InsertBlock();
    Block* InsertLargeBlock(size_t size, size_t alignment);
    void* AllocateFromBlock(Block& block, size_t size, size_t alignment);
    void RecycleBlocks();

    // memory pool
    util::ObjectPool<Block> m_block_pool;
    std::vector<Block*> m_blocks;
    Block* m_cur_block = nullptr;

    RenderQueueTaskVector m_queues[util::ecast(Queue::Count)];
    std::unordered_map<uint64_t, PerDrawcallDataWrappedErased*> m_perdrawcall_data;

    std::string m_pass_name = "ForwardBase";
    
};
}