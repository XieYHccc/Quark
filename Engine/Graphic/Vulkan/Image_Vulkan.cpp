#include "pch.h"
#include "Graphic/Vulkan/Image_Vulkan.h"
#include "Graphic/Vulkan/Device_Vulkan.h"

namespace graphic {

constexpr VkAccessFlags2 ParseImageLayoutToMemoryAccess(ImageLayout layout)
{
    switch (layout) {
    case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_2_SHADER_READ_BIT_KHR;
    case ImageLayout::GENERAL:
        return (VK_ACCESS_2_SHADER_READ_BIT_KHR & VK_ACCESS_2_SHADER_WRITE_BIT);
    case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
        return (VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT & VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return (VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    default: 
    {
        CORE_DEBUG_ASSERT("Layout type not handled")
        return 0;
    }
    }
}

Image_Vulkan::Image_Vulkan(const ImageDesc& desc)
    : Image(desc)
{

}

Image_Vulkan::Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc, const ImageInitData* init_data)
    : Image(desc), device_(device)
{
    CORE_DEBUG_ASSERT(device != nullptr)
    auto& vk_context = device_->context;
    auto vk_device = device_->vkDevice;

    // Default values
    handle_ = VK_NULL_HANDLE;
    allocation_ = VK_NULL_HANDLE;
    view_ = VK_NULL_HANDLE;
    isSwapChainImage_ = false;

    // Image create info
	VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.format = ConvertDataFormat(desc.format);
    create_info.extent.width = desc.width;
    create_info.extent.height = desc.height;
    create_info.extent.depth = desc.depth;
    create_info.mipLevels = desc.mipLevels;
    create_info.arrayLayers = desc.arraySize;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples = (VkSampleCountFlagBits)desc.samples;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.flags = 0;
    switch (desc.type) {
    case ImageType::TYPE_2D:
        create_info.imageType = VK_IMAGE_TYPE_2D;
        break;
    case ImageType::TYPE_3D:
        create_info.imageType = VK_IMAGE_TYPE_3D;
        break;
    case ImageType::TYPE_CUBE:
    {
        CORE_ASSERT(desc.arraySize == 6) // only support cube texture with 6 images now
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    }
    }

    if (vk_context->uniqueQueueFamilies.size() > 1) {
        create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = vk_context->uniqueQueueFamilies.size();
        create_info.pQueueFamilyIndices = vk_context->uniqueQueueFamilies.data();
    }
    if (desc.usageBits & IMAGE_USAGE_SAMPLING_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usageBits & IMAGE_USAGE_STORAGE_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usageBits & IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.usageBits & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (desc.usageBits & IMAGE_USAGE_CAN_COPY_FROM_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc.usageBits & IMAGE_USAGE_CAN_COPY_TO_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (desc.usageBits & IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    // Allocation info
    VmaAllocationCreateInfo allocCreateInfo = {}; // TODO: allocate a pool for small allocation
    allocCreateInfo.flags = 0;
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    VK_CHECK(vmaCreateImage(device_->vmaAllocator, &create_info, &allocCreateInfo, &handle_, &allocation_, nullptr))

    // Image view create info
    VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_create_info.image = handle_;
    image_view_create_info.format = ConvertDataFormat(desc.format);
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // TODO: Make components configurable
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.levelCount = create_info.mipLevels;
    image_view_create_info.subresourceRange.layerCount = create_info.arrayLayers;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    switch (desc.type) {
    case ImageType::TYPE_2D:
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case ImageType::TYPE_3D:
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    case ImageType::TYPE_CUBE:
    {
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    }
    }
    
    if (IsFormatSupportDepth(desc.format)) {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VK_CHECK(vkCreateImageView(vk_device, &image_view_create_info, nullptr, &view_))

    // Static data copy
    if (init_data != nullptr && (desc.usageBits & IMAGE_USAGE_SAMPLING_BIT)) {
        CORE_DEBUG_ASSERT(create_info.arrayLayers == 1 ||create_info.arrayLayers == 6)
        VkDeviceSize size = (VkDeviceSize)desc.width * desc.height * 4 * create_info.arrayLayers;
        VkDeviceSize layerSize = size / create_info.arrayLayers;
        switch (desc.format) {
        case DataFormat::R16G16B16A16_SFLOAT:
            size *= 2;
            break;
        case DataFormat::R8G8B8A8_UNORM:
            size *= 1;
            break;
        default:
            CORE_ASSERT("Format not handled yet!")
            break;
        }

        // Create staging buffer
        VkBufferCreateInfo stage_buffer_create_info{};
		stage_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stage_buffer_create_info.pNext = nullptr;
		stage_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stage_buffer_create_info.size = size;
		stage_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo stage_alloc_create_info{};
		stage_alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		stage_alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stage_buffer;
        VmaAllocation stage_allocation;
        VmaAllocationInfo stage_alloc_info;
        VK_CHECK(vmaCreateBuffer(device_->vmaAllocator, &stage_buffer_create_info, &stage_alloc_create_info, &stage_buffer, &stage_allocation, &stage_alloc_info))

        void* mapped_data = stage_alloc_info.pMappedData;
        for (size_t layer = 0; layer < create_info.arrayLayers; layer++) { // TODO: Support multiple mip levels copy
            std::memcpy((uint8_t*)mapped_data + layerSize * layer, init_data[layer].data, layerSize);
        }
        
        // Begin copy cmd
        auto& transferCmd = device_->transferCmd;
        VkCommandBuffer copy_cmd = transferCmd.begin_immediate_submit();

        // Transit image to transfer dst format
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.image = handle_;
        barrier.oldLayout = create_info.initialLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = create_info.mipLevels;
        barrier.subresourceRange.baseArrayLayer =0;
        barrier.subresourceRange.layerCount = create_info.arrayLayers;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(copy_cmd, &dependencyInfo);

        // Copy info
        VkBufferImageCopy copy;
    	copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = 0; //TODO: Support multiple mips copy
        copy.imageSubresource.baseArrayLayer =0;
        copy.imageSubresource.layerCount = create_info.arrayLayers;
        copy.imageOffset = { 0, 0, 0 };
        copy.imageExtent = { desc.width, desc.height , desc.depth }; 
        vkCmdCopyBufferToImage(copy_cmd, stage_buffer, handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        // Transit image layout to required init layout
        std::swap(barrier.srcStageMask, barrier.dstStageMask);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = ConvertImageLayout(desc_.initialLayout);
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = ParseImageLayoutToMemoryAccess(desc.initialLayout);
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(copy_cmd, &dependencyInfo);

        // submit and block cpu
        transferCmd.block_submit();
        vmaDestroyBuffer(device->vmaAllocator, stage_buffer, stage_allocation);
    }
    else if (desc.initialLayout != ImageLayout::UNDEFINED) {    // Transit layout to required init layout
        auto& transferCmd = device_->transferCmd;
        VkCommandBuffer cmd = transferCmd.begin_immediate_submit();

        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = handle_;
        barrier.oldLayout = create_info.initialLayout;
        barrier.newLayout = ConvertImageLayout(desc.initialLayout);
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = ParseImageLayoutToMemoryAccess(desc.initialLayout);
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = create_info.arrayLayers;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = create_info.mipLevels;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        if (IsFormatSupportDepth(desc.format)) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);
        transferCmd.block_submit();
    }
}

Image_Vulkan::~Image_Vulkan()
{
    if (isSwapChainImage_) {
        return;
    }

    auto& frame = device_->GetCurrentFrame();
    if (handle_ != VK_NULL_HANDLE) {
        frame.garbageImages.push_back(std::make_pair(handle_, allocation_));
    }

    if (view_) {
        frame.grabageViews.push_back(view_);
    }

}
}