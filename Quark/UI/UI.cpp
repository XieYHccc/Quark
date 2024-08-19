#include "Quark/qkpch.h"
#include "Quark/UI/UI.h"

#ifdef USE_VULKAN_DRIVER
#include "Quark/UI/Vulkan/UI_Vulkan.h"
#endif

namespace quark {

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
}