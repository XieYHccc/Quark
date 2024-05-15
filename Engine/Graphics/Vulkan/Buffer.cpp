#include "pch.h"
#include "Graphics/Vulkan/Buffer.h"
#include "Graphics/Vulkan/Context.h"

namespace vk {

void Buffer::DestroyBuffer(Context& context, Buffer buffer)
{
    vmaDestroyBuffer(context.GetVmaAllocator(), buffer.vkBuffer, buffer.allocation);
}

Buffer Buffer::CreateStagingBuffer(Context& context, VkDeviceSize size)
{
    BufferBuilder builder;
    builder.SetSize(size)
        .SetUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        .SetVmaUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
        .SetVmaFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT);
    
    return builder.Build(context);
}

void BufferBuilder::Clear()
{
    createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0};
    vmaAllocationInfo = {};
}

Buffer BufferBuilder::Build(Context& context) const
{
    Buffer newBuffer;

    vmaCreateBuffer(context.GetVmaAllocator(), &createInfo, &vmaAllocationInfo, &newBuffer.vkBuffer, &newBuffer.allocation, &newBuffer.info);
    newBuffer.usage = createInfo.usage;
    
    return newBuffer;
}

}
