#pragma once

#include <stddef.h>
#include <utility>
#include <memory>
#include <atomic>
#include <type_traits>

namespace quark::util
{
class SingleThreadCounter
{
public:
	inline void add_ref()
	{
		count++;
	}

	inline bool release()
	{
		return --count == 0;
	}

private:
	uint32_t count = 1;
};

class MultiThreadCounter
{
public:
	MultiThreadCounter()
	{
		count.store(1, std::memory_order_relaxed);
	}

	inline void add_ref()
	{
		count.fetch_add(1, std::memory_order_relaxed);
	}

	inline bool release()
	{
		auto result = count.fetch_sub(1, std::memory_order_acq_rel);
		return result == 1;
	}

private:
	std::atomic_uint32_t count;
};

template <typename T>
class IntrusivePtr;

template <typename T, typename Deleter = std::default_delete<T>, typename ReferenceOps = SingleThreadCounter>
class IntrusivePtrEnabled{
	template<typename >
	friend class IntrusivePtr;
public:
	using IntrusivePtrType = IntrusivePtr<T>;
	using EnabledBase = T;
	using EnabledDeleter = Deleter;
	using EnabledReferenceOp = ReferenceOps;

	IntrusivePtrEnabled() = default;

	IntrusivePtrEnabled(const IntrusivePtrEnabled &) = delete;

	void operator=(const IntrusivePtrEnabled &) = delete;

protected:
	util::IntrusivePtr<T> reference_from_this();

private:
	void release_reference()
	{
		if (reference_count.release())
			Deleter()(static_cast<T *>(this));
	}

	void add_reference()
	{
		reference_count.add_ref();
	}

	ReferenceOps reference_count;
};

template <typename T>
class IntrusivePtr
{
public:
	template <typename U>
	friend class IntrusivePtr;

	IntrusivePtr() = default;

	explicit IntrusivePtr(T *handle)
		: data(handle)
	{
	}

	T &operator*()
	{
		return *data;
	}

	const T &operator*() const
	{
		return *data;
	}

	T *operator->()
	{
		return data;
	}

	const T *operator->() const
	{
		return data;
	}

	explicit operator bool() const
	{
		return data != nullptr;
	}

	bool operator==(const IntrusivePtr &other) const
	{
		return data == other.data;
	}

	bool operator!=(const IntrusivePtr &other) const
	{
		return data != other.data;
	}

	T *get()
	{
		return data;
	}

	const T *get() const
	{
		return data;
	}

	void reset()
	{
		using ReferenceBase = IntrusivePtrEnabled<
				typename T::EnabledBase,
				typename T::EnabledDeleter,
				typename T::EnabledReferenceOp>;

		// Static up-cast here to avoid potential issues with multiple intrusive inheritance.
		// Also makes sure that the pointer type actually inherits from this type.
		if (data)
			static_cast<ReferenceBase *>(data)->release_reference();
		data = nullptr;
	}

	template <typename U>
	IntrusivePtr &operator=(const IntrusivePtr<U> &other)
	{
		static_assert(std::is_base_of<T, U>::value,
		              "Cannot safely assign downcasted intrusive pointers.");

		using ReferenceBase = IntrusivePtrEnabled<
				typename T::EnabledBase,
				typename T::EnabledDeleter,
				typename T::EnabledReferenceOp>;

		reset();
		data = static_cast<T *>(other.data);

		// Static up-cast here to avoid potential issues with multiple intrusive inheritance.
		// Also makes sure that the pointer type actually inherits from this type.
		if (data)
			static_cast<ReferenceBase *>(data)->add_reference();
		return *this;
	}

	IntrusivePtr &operator=(const IntrusivePtr &other)
	{
		using ReferenceBase = IntrusivePtrEnabled<
				typename T::EnabledBase,
				typename T::EnabledDeleter,
				typename T::EnabledReferenceOp>;

		if (this != &other)
		{
			reset();
			data = other.data;
			if (data)
				static_cast<ReferenceBase *>(data)->add_reference();
		}
		return *this;
	}

	template <typename U>
	IntrusivePtr(const IntrusivePtr<U> &other)
	{
		*this = other;
	}

	IntrusivePtr(const IntrusivePtr &other)
	{
		*this = other;
	}

	~IntrusivePtr()
	{
		reset();
	}

	template <typename U>
	IntrusivePtr &operator=(IntrusivePtr<U> &&other) noexcept
	{
		reset();
		data = other.data;
		other.data = nullptr;
		return *this;
	}

	IntrusivePtr &operator=(IntrusivePtr &&other) noexcept
	{
		if (this != &other)
		{
			reset();
			data = other.data;
			other.data = nullptr;
		}
		return *this;
	}

	template <typename U>
	IntrusivePtr(IntrusivePtr<U> &&other) noexcept
	{
		*this = std::move(other);
	}

	template <typename U>
	IntrusivePtr(IntrusivePtr &&other) noexcept
	{
		*this = std::move(other);
	}

	T *release() &
	{
		T *ret = data;
		data = nullptr;
		return ret;
	}

	T *release() &&
	{
		T *ret = data;
		data = nullptr;
		return ret;
	}

private:
	T *data = nullptr;
};

template <typename T, typename Deleter, typename ReferenceOps>
IntrusivePtr<T> IntrusivePtrEnabled<T, Deleter, ReferenceOps>::reference_from_this()
{
	add_reference();
	return IntrusivePtr<T>(static_cast<T *>(this));
}

template <typename Derived>
using DerivedIntrusivePtrType = IntrusivePtr<Derived>;

template <typename T, typename... P>
DerivedIntrusivePtrType<T> make_handle(P &&... p)
{
	return DerivedIntrusivePtrType<T>(new T(std::forward<P>(p)...));
}

template <typename Base, typename Derived, typename... P>
typename Base::IntrusivePtrType make_derived_handle(P &&... p)
{
	return typename Base::IntrusivePtrType(new Derived(std::forward<P>(p)...));
}

template <typename T>
using ThreadSafeIntrusivePtrEnabled = IntrusivePtrEnabled<T, std::default_delete<T>, MultiThreadCounter>;
}