#include "pch.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Buffer.h"
#include "Graphics/Vulkan/Context.h"
#include "Graphics/Vulkan/Initializers.h"

namespace vk {

Image Image::CreateTexImage(Context& context, void* data, uint32_t width, uint32_t height, bool mipmapped)
{
    ImageBuilder builder;
    builder.SetExtent(VkExtent3D{width, height, 1})
        .SetFormat(VK_FORMAT_R8G8B8A8_UNORM)
        .SetUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        .EnableImplicitSharingMode()
        .SetVmaUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetVmaRequiredFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mipmapped) {
        builder.EnableMipmapped();
    }
    Image newImage = builder.Build(context);

    // create staging buffer for copying
    size_t dataSize = width * height * 4;
    Buffer staging = Buffer::CreateStagingBuffer(context, dataSize);
    memcpy(staging.info.pMappedData, data, dataSize);

    context.ImmediateSubmit([&](VkCommandBuffer cmd) {
		vk::Image::TransitImageLayout(context, cmd, newImage.vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = VkExtent3D{width, height, 1};

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, staging.vkBuffer, newImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		vk::Image::TransitImageLayout(context, cmd, newImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    vk::Buffer::DestroyBuffer(context, staging);
    return newImage;
}

void Image::DestroyImage(Context& context, Image image)
{
    vmaDestroyImage(context.GetVmaAllocator(), image.vkImage, image.allocation);
    vkDestroyImageView(context.GetVkDevice(), image.imageView, nullptr);
}

void Image::TransitImageLayout(Context& context, VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.pNext = nullptr;

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = vk::init::image_subresource_range(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo depInfo {};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;
    //auto pVkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR");
    context.extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &depInfo);
}

void Image::CopyImage(Context& context, VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

    // auto pVkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(device, "vkCmdBlitImage2KHR");
	context.extendFunction.pVkCmdBlitImage2KHR(cmd, &blitInfo);
}

void ImageBuilder::Clear()
{
    createInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        {},
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT
    };

    vmaAllocationInfo = {};
    mipmapped = false;
}

Image ImageBuilder::Build(Context& context)
{
    // Create image
    Image newImage;
    if (mipmapped) {
        createInfo.mipLevels = static_cast<uint32_t>(
            std::floor(std::log2(std::max(createInfo.extent.width, createInfo.extent.height)))) + 1;
    }
    VK_ASSERT(vmaCreateImage(context.GetVmaAllocator(), &createInfo, &vmaAllocationInfo, &newImage.vkImage, &newImage.allocation, &newImage.info));

    newImage.imageExtent = createInfo.extent;
    newImage.mipLevels= createInfo.mipLevels;
    newImage.imageFormat = createInfo.format;
    newImage.usage = createInfo.usage;
    // Create image views
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (createInfo.format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	VkImageViewCreateInfo view_info = vk::init::imageview_create_info(createInfo.format, newImage.vkImage, aspectFlag);
	view_info.subresourceRange.levelCount = createInfo.mipLevels;

	VK_ASSERT(vkCreateImageView(context.GetVkDevice(), &view_info, nullptr, &newImage.imageView));
    return newImage;
}
}