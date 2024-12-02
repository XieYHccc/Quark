#pragma once
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Buffer.h"

namespace quark::rhi {

class Buffer_Vulkan : public Buffer {
public:
    ~Buffer_Vulkan();
    Buffer_Vulkan(Device_Vulkan* device, const BufferDesc& desc, const void* init_data = nullptr);

    const VkBuffer GetHandle() const { return m_Handle; }
    
private:
    Device_Vulkan* m_Device = nullptr;
    VkBuffer m_Handle = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_AllocInfo = {};

};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Buffer)
}