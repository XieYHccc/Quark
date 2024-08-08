#pragma once

#include "Hash.h"
#include "ObjectPool.h"
#include "IntrusiveList.h"
#include "IntrusiveHashMap.h"
#include <vector>

namespace quark::util {
template <typename T>
class TemporaryHashmapEnabled
{
public:
	void set_hash(Hash hash_)
	{
		hash = hash_;
	}

	void set_index(unsigned index_)
	{
		index = index_;
	}

	Hash get_hash()
	{
		return hash;
	}

	unsigned get_index() const
	{
		return index;
	}

private:
	Hash hash = 0;
	unsigned index = 0;
};

template <typename T, unsigned RingSize = 4, bool ReuseObjects = false>
class TemporaryHashmap
{
public:
	~TemporaryHashmap()
	{
		clear();
	}

	void clear()
	{
		for (auto &ring : rings)
		{
			while (!ring.empty())
			{
				auto itr = ring.begin();
				ring.erase(itr);
				auto &node = *itr;
				object_pool.free(static_cast<T *>(&node));
			}
		}
		hashmap.clear();

		for (auto &vacant : vacants)
			object_pool.free(static_cast<T *>(&*vacant));
		vacants.clear();
		object_pool.clear();
	}

	void begin_frame()
	{
		index = (index + 1) & (RingSize - 1);
		auto &ring = rings[index];

		while (!ring.empty())
		{
			auto itr = ring.begin();
			ring.erase(itr);
			auto &node = *itr;
			hashmap.erase(node.get_hash());
			free_object(&node, ReuseTag<ReuseObjects>());
		}
	}

	T *request(Hash hash)
	{
		auto *v = hashmap.find(hash);
		if (v)
		{
			auto node = v->get();
			if (node->get_index() != index)
			{
				rings[index].move_to_front(rings[node->get_index()], node);
				node->set_index(index);
			}

			return &*node;
		}
		else
			return nullptr;
	}

	template <typename... P>
	void make_vacant(P &&... p)
	{
		vacants.push_back(object_pool.allocate(std::forward<P>(p)...));
	}

	T *request_vacant(Hash hash)
	{
		if (vacants.empty())
			return nullptr;

		auto top = vacants.back();
		vacants.pop_back();
		top->set_index(index);
		top->set_hash(hash);
		hashmap.emplace_replace(hash, top);
		rings[index].insert_front(top);
		return &*top;
	}

	template <typename... P>
	T *emplace(Hash hash, P &&... p)
	{
		auto *node = object_pool.allocate(std::forward<P>(p)...);
		node->set_index(index);
		node->set_hash(hash);
		hashmap.emplace_replace(hash, node);
		rings[index].insert_front(node);
		return node;
	}

private:
	IntrusiveList<T> rings[RingSize];
	ObjectPool<T> object_pool;
	unsigned index = 0;
	IntrusiveHashMap<IntrusivePODWrapper<typename IntrusiveList<T>::Iterator>> hashmap;
	std::vector<typename IntrusiveList<T>::Iterator> vacants;

	template <bool reuse>
	struct ReuseTag
	{
	};

	void free_object(T *object, const ReuseTag<false> &)
	{
		object_pool.free(object);
	}

	void free_object(T *object, const ReuseTag<true> &)
	{
		vacants.push_back(object);
	}
};
}