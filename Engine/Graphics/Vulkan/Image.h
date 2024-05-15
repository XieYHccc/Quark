#pragma once
#include "Graphics/Vulkan/Assert.h"

namespace vk {

class Context;
struct Image {
    VkImage vkImage = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkExtent3D imageExtent {};
    VkFormat imageFormat {};
    VkImageUsageFlags usage {};
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo info {};
    uint32_t mipLevels {1};


	static Image CreateTexImage(Context& context, void* data, uint32_t width, uint32_t height, bool mipmapped);

	// Some helpers for commands recording
	static void TransitImageLayout(Context& context, VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	static void CopyImage(Context&, VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

    static void DestroyImage(Context& context, Image image);
};

struct ImageBuilder {
    bool mipmapped;
    VkImageCreateInfo createInfo;
    VmaAllocationCreateInfo vmaAllocationInfo;

	ImageBuilder() { Clear(); }
	void Clear();
	Image Build(Context& context);

	// Settings
	ImageBuilder& SetExtent(VkExtent3D extent)
	{
		createInfo.extent = extent;
		return *this;
	}

    ImageBuilder& SetFormat(VkFormat format)
	{
		createInfo.format = format;
		return *this;
	}

	ImageBuilder& SetUsage(VkImageUsageFlags usage)
	{
		createInfo.usage = usage;
		return *this;
	}

	ImageBuilder& SetQueueFamilies(const std::vector<uint32_t>& queue_families)
	{
		createInfo.queueFamilyIndexCount = queue_families.size();
		createInfo.pQueueFamilyIndices   = queue_families.data();
		return *this;
	}

	ImageBuilder& SetSharingMode(VkSharingMode sharing)
	{
		createInfo.sharingMode = sharing;
		return *this;
	}

	ImageBuilder& EnableImplicitSharingMode()
	{
		if (createInfo.queueFamilyIndexCount != 0)
			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		else
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		return *this;
	}

	ImageBuilder& EnableMipmapped()
	{
        mipmapped = true;
		return *this;
	}

	ImageBuilder& SetSampleCount(VkSampleCountFlagBits sample_count)
	{
		createInfo.samples = sample_count;
		return *this;
	}

	ImageBuilder& SetTiling(VkImageTiling tiling)
	{
		createInfo.tiling = tiling;
		return *this;
	}
	ImageBuilder& SetVmaUsage(VmaMemoryUsage usage)
	{
		vmaAllocationInfo.usage = usage;
		return *this;
	}

	ImageBuilder& SetVmaFlags(VmaAllocationCreateFlags flags)
	{
		vmaAllocationInfo.flags = flags;
		return *this;
	}

	ImageBuilder& SetVmaRequiredFlags(VkMemoryPropertyFlags flags)
	{
		vmaAllocationInfo.requiredFlags = flags;
		return *this;
	}

    ImageBuilder& SetVmaPreferredFlags(VkMemoryPropertyFlags flags)
	{
		vmaAllocationInfo.preferredFlags = flags;
		return *this;
	}
    
};

}