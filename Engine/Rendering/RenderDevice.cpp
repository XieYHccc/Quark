#include "pch.h"
#include "Rendering/RenderDevice.h"

#ifdef  USE_VULKAN_DRIVER
#include "Rendering/Vulkan/RenderDevice_Vulkan.h"
#endif

RenderDevice::RenderDevice()
{
}

RenderDevice::~RenderDevice()
{
}

template<>
template<>
RenderDevice& MakeSingletonPtr<RenderDevice>::CreateSingleton()
{
#ifdef USE_VULKAN_DRIVER
    m_global = new RenderDevice_Vulkan();
    return *m_global;
#endif
}
