#include "pch.h"
#include "UI/UI.h"

#ifdef USE_VULKAN_DRIVER
#include "UI/Vulkan/UI_Vulkan.h"
#endif

template <>
template <>
UI* util::MakeSingletonPtr<UI>::CreateSingleton()
{
#ifdef USE_VULKAN_DRIVER
    m_global = new UI_Vulkan();
    return m_global;
#else
    CORE_ASSERT(0, "No UI driver is defined");
#endif

}