#include "Quark/qkpch.h"
#include "Quark/Graphic/Vulkan/Buffer_Vulkan.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"

namespace quark::graphic {

Buffer_Vulkan::~Buffer_Vulkan()
{
    if (m_Handle != VK_NULL_HANDLE) {
        auto& frame = m_Device->GetCurrentFrame();
        frame.garbageBuffers.push_back(std::make_pair(m_Handle, m_Allocation)); 
    }
}

Buffer_Vulkan::Buffer_Vulkan(Device_Vulkan* device, const BufferDesc& desc, const void* init_data)
    : Buffer(desc), m_Device(device)
{
    CORE_DEBUG_ASSERT(m_Device != nullptr)
    CORE_DEBUG_ASSERT(desc.size != 0)

    const auto& vulkan_context = m_Device->vkContext;

    // Buffer create info
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = desc.size;
    buffer_create_info.usage = desc.usageBits;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vulkan_context->features12.bufferDeviceAddress)
        buffer_create_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (m_Device->vkContext->uniqueQueueFamilies.size() > 1) {
        buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        buffer_create_info.queueFamilyIndexCount = vulkan_context->uniqueQueueFamilies.size();
        buffer_create_info.pQueueFamilyIndices = vulkan_context->uniqueQueueFamilies.data();
    }

    // Memory allocation info
    VmaAllocationCreateInfo alloc_create_info = {};
    if (desc.domain == BufferMemoryDomain::CPU) {
        bool is_src = (desc.usageBits & BUFFER_USAGE_TRANSFER_FROM_BIT? true : false);
        bool is_dst = (desc.usageBits & BUFFER_USAGE_TRANSFER_TO_BIT? true : false);
        if (is_dst && !is_src) {
            // Looks like a readback buffer: GPU copies from VRAM, then CPU maps and reads.
            alloc_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        }
        else {
            // Staging buffer or just want to write from cpu but read directly from gpu
            alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        alloc_create_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		alloc_create_info.requiredFlags = (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    else if (desc.domain == BufferMemoryDomain::GPU) 
    {
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        // TODO: Optimize small allocation with dedicated pool
    } 

    // Create Buffer
    VK_CHECK(vmaCreateBuffer(m_Device->vmaAllocator, &buffer_create_info, &alloc_create_info, &m_Handle, &m_Allocation, &m_AllocInfo))

    if (desc.domain == BufferMemoryDomain::CPU)
        m_pMappedData = m_AllocInfo.pMappedData;

    if (vulkan_context->features12.bufferDeviceAddress) {
        VkBufferDeviceAddressInfo bda_info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        bda_info.buffer = m_Handle;
        m_GpuAddress = vkGetBufferDeviceAddress(m_Device->vkDevice, &bda_info); 
    }

    // Data Copy
    if (init_data != nullptr) 
    {
        if (desc.domain == BufferMemoryDomain::CPU) 
        {
            memcpy(m_pMappedData, init_data, desc.size);
        }
        else 
        {  // static data uplodaing

            Device_Vulkan::CopyCmdAllocator::CopyCmd copyCmd = m_Device->copyAllocator.allocate(desc.size);
            memcpy(copyCmd.stageBuffer->GetMappedDataPtr(), init_data, m_desc.size);

            // copy buffer
            VkBufferCopy copyRegion = {};
            copyRegion.size = desc.size;
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            vkCmdCopyBuffer(copyCmd.transferCmdBuffer, ToInternal(copyCmd.stageBuffer.get()).m_Handle, m_Handle, 1, &copyRegion);

            // Submit this command which would block CPU
            m_Device->copyAllocator.submit(copyCmd);
        }
    }

}

}