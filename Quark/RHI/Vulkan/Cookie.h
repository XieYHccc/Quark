#pragma once
#include "Quark/RHI/Vulkan/Common_Vulkan.h"

namespace quark::rhi
{
class Device_Vulkan;
class Cookie
{
public:
	Cookie(Device_Vulkan* device);

	uint64_t GetCookie() const
	{
		return m_cookie;
	}

private:
	uint64_t m_cookie;
};

class InternalSyncEnabled
{
public:
	void SetInternalSynced()
	{
		m_internal_sync = true;
	}

protected:
	bool m_internal_sync = false;
};
}