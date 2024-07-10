#include "pch.h"
#include "Buffer_Vulkan.h"
#include "Device_Vulkan.h"

namespace graphic {

Buffer_Vulkan::~Buffer_Vulkan()
{
    if (handle_ != VK_NULL_HANDLE) {
        auto& frame = device_->GetCurrentFrame();
        frame.garbageBuffers.push_back(std::make_pair(handle_, allocation_)); 
    }
}

Buffer_Vulkan::Buffer_Vulkan(Device_Vulkan* device, const BufferDesc& desc, const void* init_data)
    : Buffer(desc), device_(device)
{
    CORE_DEBUG_ASSERT(device_ != nullptr)
    CORE_DEBUG_ASSERT(desc.size != 0)
    auto& vulkan_context = device_->context;
    auto& vmaAllocator = device->vmaAllocator;
    VkDevice vk_device = device_->vkDevice;
    // Buffer create info
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = desc.size;
    buffer_create_info.usage = 0;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    switch (desc_.type) {
    case BufferType::INDEX_BUFFER:
        buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case BufferType::STORAGE_BUFFER:
        buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;
    case BufferType::UNIFORM_BUFFER:
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    case BufferType::VERTEX_BUFFER:
        buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    }
    buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (vulkan_context->features12.bufferDeviceAddress) {
        buffer_create_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    if (device_->context->uniqueQueueFamilies.size() > 1) {
        buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        buffer_create_info.queueFamilyIndexCount = vulkan_context->uniqueQueueFamilies.size();
        buffer_create_info.pQueueFamilyIndices = vulkan_context->uniqueQueueFamilies.data();
    }

    // Memory allocation info
    VmaAllocationCreateInfo alloc_create_info = {};
    if (desc.domain == BufferMemoryDomain::CPU) {
        alloc_create_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		alloc_create_info.requiredFlags = (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    else if (desc.domain == BufferMemoryDomain::GPU) {
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        // TODO: Optimize small allocation with dedicated pool
    } 
    VK_CHECK(vmaCreateBuffer(vmaAllocator, &buffer_create_info, &alloc_create_info, &handle_, &allocation_, &allocInfo_))

    if (desc.domain == BufferMemoryDomain::CPU) {
        pMappedData_ = allocInfo_.pMappedData;
    }
    if (vulkan_context->features12.bufferDeviceAddress) {
        VkBufferDeviceAddressInfo bda_info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        bda_info.buffer = handle_;
        gpuAddress_ = vkGetBufferDeviceAddress(vk_device, &bda_info); 
    }

    // Data Copy
    if (init_data != nullptr) {
        if (desc.domain == BufferMemoryDomain::CPU) {
            memcpy(pMappedData_, init_data, desc.size);
        }
        else {  // static data transfer
            
            // begin
            VkCommandBuffer copy_cmd = device_->transferCmd.begin_immediate_submit();
            // create staging buffer //TODO: Cache staging buffer
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VmaAllocationCreateInfo stage_alloc_info = {};
            stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            VkBuffer stage_buffer;
            VmaAllocation stage_allocation;
            VmaAllocationInfo stage_info = {};
            VK_CHECK(vmaCreateBuffer(vmaAllocator, &buffer_create_info, &stage_alloc_info, &stage_buffer, &stage_allocation, &stage_info))
            memcpy(stage_info.pMappedData, init_data, desc_.size);

            // copy buffer
            VkBufferCopy copyRegion = {};
            copyRegion.size = desc.size;
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            vkCmdCopyBuffer( copy_cmd, stage_buffer, handle_, 1, &copyRegion);

            // Submit this command which would block CPU
            device_->transferCmd.block_submit();
            vmaDestroyBuffer(vmaAllocator, stage_buffer, stage_allocation);
        }
    }

}

}