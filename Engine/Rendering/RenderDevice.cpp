#include "pch.h"
#include "Rendering/RenderDevice.h"

#ifdef USE_VULKAN_DRIVER
#include "Rendering/Vulkan/RenderDeviceDriver_Vulkan.h"
#endif

bool RenderDevice::Init()
{
    // Create backend
#ifdef USE_VULKAN_DRIVER
    m_Backend = new RenderDeviceDriver_Vulkan();
#endif
    m_Backend->Init();
    return true;
}

void RenderDevice::ShutDown()
{
    // Delete backend
    m_Backend->ShutDown();
    delete m_Backend;
}

bool RenderDevice::BeiginFrame(f32 deltaTime)
{
    return true;
}

bool RenderDevice::EndFrame(f32 deltaTime)
{
    return true;
}