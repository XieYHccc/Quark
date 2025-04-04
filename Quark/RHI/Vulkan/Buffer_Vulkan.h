#pragma once
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Buffer.h"
#include "Quark/RHI/Vulkan/Cookie.h"

namespace quark::rhi {

class Buffer_Vulkan : public Buffer, public Cookie, public InternalSyncEnabled {
public:
    ~Buffer_Vulkan();
    Buffer_Vulkan(Device_Vulkan* device, const BufferDesc& desc, const void* init_data = nullptr);

    const VkBuffer GetHandle() const { return m_handle; }
    
private:
    Device_Vulkan* m_device = nullptr;
    VkBuffer m_handle = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_allocInfo = {};
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Buffer)
}