#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/Cookie.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi
{
Cookie::Cookie(Device_Vulkan* device)
	: m_cookie(device->AllocateCookie())
{
}

}