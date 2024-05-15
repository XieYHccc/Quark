#pragma once
#include "Graphics/Vulkan/Assert.h"

namespace vk {

class Context;

struct Buffer {
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo info {};
	VkBufferUsageFlags usage;
	

	static Buffer CreateStagingBuffer(Context& context, VkDeviceSize size);
	static void DestroyBuffer(Context& context, Buffer buffer);
};

struct BufferBuilder
{
	VkBufferCreateInfo createInfo;
    VmaAllocationCreateInfo vmaAllocationInfo;

	BufferBuilder() { Clear();}
	void Clear();
	
	Buffer Build(Context& context) const;
	
	// Settings
	BufferBuilder& SetSize(VkDeviceSize size)
	{
		createInfo.size = size;
		return *this;
	}

	BufferBuilder& SetUsage(VkBufferUsageFlags usage)
	{
		createInfo.usage = usage;
		return *this;
	}

	BufferBuilder& SetVmaUsage(VmaMemoryUsage usage)
	{
		vmaAllocationInfo.usage = usage;
		return *this;
	}

	BufferBuilder& SetVmaFlags(VmaAllocationCreateFlags flags)
	{
		vmaAllocationInfo.flags = flags;
		return *this;
	}

	BufferBuilder& SetVmaRequiredFlags(VkMemoryPropertyFlags flags)
	{
		vmaAllocationInfo.requiredFlags = flags;
		return *this;
	}

    BufferBuilder& SetVmaPreferredFlags(VkMemoryPropertyFlags flags)
	{
		vmaAllocationInfo.preferredFlags = flags;
		return *this;
	}
    
};

}