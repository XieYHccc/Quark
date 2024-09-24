#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <stdlib.h>

#include "AlignedAlloc.h"

//#define OBJECT_POOL_DEBUG

namespace quark::util {

template<typename T>
class ObjectPool
{
public:
	template<typename... P>
	T *allocate(P &&... p)
	{
#ifndef OBJECT_POOL_DEBUG
		if (vacants.empty())
		{
			unsigned num_objects = 64u << memory.size();
			T *ptr = static_cast<T *>(memalign_alloc(std::max<size_t>(64, alignof(T)),
			                                         num_objects * sizeof(T)));
			if (!ptr)
				return nullptr;

			for (unsigned i = 0; i < num_objects; i++)
				vacants.push_back(&ptr[i]);

			memory.emplace_back(ptr);
		}

		T *ptr = vacants.back();
		vacants.pop_back();
		new(ptr) T(std::forward<P>(p)...);
		return ptr;
#else
		return new T(std::forward<P>(p)...);
#endif
	}

	void free(T *ptr)
	{
#ifndef OBJECT_POOL_DEBUG
		ptr->~T();
		vacants.push_back(ptr);
#else
		delete ptr;
#endif
	}

	void clear()
	{
#ifndef OBJECT_POOL_DEBUG
		vacants.clear();
		memory.clear();
#endif
	}

protected:
#ifndef OBJECT_POOL_DEBUG
	std::vector<T *> vacants;

	struct MallocDeleter
	{
		void operator()(T *ptr)
		{
			memalign_free(ptr);
		}
	};

	std::vector<std::unique_ptr<T, MallocDeleter>> memory;
#endif
};

template<typename T>
class ThreadSafeObjectPool : private ObjectPool<T>
{
public:
	template<typename... P>
	T *allocate(P &&... p)
	{
		std::lock_guard<std::mutex> holder{lock};
		return ObjectPool<T>::allocate(std::forward<P>(p)...);
	}

	void free(T *ptr)
	{
#ifndef OBJECT_POOL_DEBUG
		ptr->~T();
		std::lock_guard<std::mutex> holder{lock};
		this->vacants.push_back(ptr);
#else
		delete ptr;
#endif
	}

	void clear()
	{
		std::lock_guard<std::mutex> holder{lock};
		ObjectPool<T>::clear();
	}

private:
	std::mutex lock;
};
}