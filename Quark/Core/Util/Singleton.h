#pragma once
#include "Quark/Core/Base.h"

namespace quark::util {

// If class inherits that it will become a singleton.
template<typename T>
class MakeSingleton
{
public:
	QK_FORCE_INLINE static T& Singleton()
	{
		CORE_DEBUG_ASSERT(m_initialized);
		return *reinterpret_cast<T*>(m_global);
	}

	template<typename... TArgs>
	static T& CreateSingleton(TArgs... args)
	{
		CORE_DEBUG_ASSERT(!m_initialized);
		::new(m_global) T(args...);
		m_initialized = true;

		return *reinterpret_cast<T*>(m_global);
	}

	static void FreeSingleton()
	{
		if(m_initialized)
		{
			reinterpret_cast<T*>(m_global)->~T();
			m_initialized = false;
		}
	}

	static bool IsAllocated()
	{
		return m_initialized;
	}

private:
	static u8 m_global[];
	static inline bool m_initialized = false;
};

template<typename T>
alignas(QK_SAFE_ALIGNMENT) u8 MakeSingleton<T>::m_global[sizeof(T)];

/// If class inherits that it will become a singleton.
template<typename T>
class MakeSingletonPtr
{
public:
	QK_FORCE_INLINE static T* Singleton()
	{
		return m_global;
	}

	template<typename... TArgs>
	static T* CreateSingleton(TArgs... args)
	{
		CORE_ASSERT(m_global == nullptr);
		m_global = new T(args...);

		return m_global;
	}

	static void FreeSingleton()
	{
		if(m_global)
		{
			delete m_global;
			m_global = nullptr;
		}
	}

	static bool IsAllocated()
	{
		return m_global != nullptr;
	}

private:
	static inline T* m_global = nullptr;
};

}