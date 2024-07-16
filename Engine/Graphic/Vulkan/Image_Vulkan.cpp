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

void TextureFormatLayout::SetUp2D(DataFormat format, uint32_t width, uint32_t height, uint32_t array_size, uint32_t mip_levels)
{
    format_ = format;
    image_type_ = ImageType::TYPE_2D;
    array_size_ = array_size;
    mip_levels_ = mip_levels;
    
    FillMipInfos(width, height, 1);
    
}

void TextureFormatLayout::FillMipInfos(uint32_t width, uint32_t height, uint32_t depth)
{
	block_stride_ = GetFormatStride(format_);
	GetFormatBlockDim(format_, block_dim_x_, block_dim_y_);

    // generate mipmaps
	if (mip_levels_ == 0){
        uint32_t size = unsigned(std::max(std::max(width, height), depth));
	    uint32_t levels = 0;
        while (size) {
            levels++;
            size >>= 1;
        }
        mip_levels_ = levels;
    }

	size_t offset = 0;
	for (uint32_t mip = 0; mip < mip_levels_; mip++)
	{
		offset = (offset + 15) & ~15;

		uint32_t blocks_x = (width + block_dim_x_ - 1) / block_dim_x_;
		uint32_t blocks_y = (height + block_dim_y_ - 1) / block_dim_y_;
		size_t mip_size = blocks_x * blocks_y * array_size_ * depth * block_stride_;

		mips_[mip].offset = offset;
		mips_[mip].num_block_x = blocks_x;
		mips_[mip].num_block_y = blocks_y;
		mips_[mip].row_length = blocks_x * block_dim_x_;
		mips_[mip].image_height = blocks_y * block_dim_y_;
		mips_[mip].width = width;
		mips_[mip].height = height;
		mips_[mip].depth = depth;

		offset += mip_size;
		width = std::max((width >> 1u), 1u);
		height = std::max((height >> 1u), 1u);
		depth = std::max((depth >> 1u), 1u);
	}

	required_size_ = offset;
}

Image_Vulkan::Image_Vulkan(const ImageDesc& desc)
    : Image(desc)
{

}

void Image_Vulkan::PrepareCopy(const ImageDesc& desc, const TextureFormatLayout& layout, const ImageInitData* init_data, Ref<Buffer> stage_buffer, std::vector<VkBufferImageCopy>& copys)
{
    CORE_DEBUG_ASSERT(copys.empty())
    CORE_DEBUG_ASSERT(stage_buffer->GetDesc().size >= layout.GetRequiredSize())

    // Get mapped data ptr
    void* mapped = stage_buffer->GetMappedDataPtr();

    size_t index = 0;
    // loop per mipmap level to copy data into staging buffer
    for (size_t level = 0; level < layout.GetMipLevels(); level++) {
        const auto& mip_info = layout.GetMipInfo(level);
        size_t dst_row_size = mip_info.num_block_x * layout.GetBlockStride();
        size_t dst_slice_size = dst_row_size * mip_info.num_block_y;

        for (size_t layer = 0; layer < desc.arraySize; layer++, index++) {
            const ImageInitData& sub_resouce = init_data[index];
            size_t src_row_length = (sub_resouce.row_length != UINT32_MAX? sub_resouce.row_length : mip_info.row_length);
            size_t src_image_height = (sub_resouce.image_height != UINT32_MAX? sub_resouce.image_height : mip_info.image_height);
            
            size_t src_row_size = ((src_row_length + layout.GetBlockDimX() - 1) / layout.GetBlockDimX()) * layout.GetBlockStride();
            size_t src_slice_size = ((src_image_height + layout.GetBlockDimY() - 1) / layout.GetBlockDimY()) * src_row_size;

            uint8_t* dst = static_cast<uint8_t*>(mapped) + mip_info.offset + dst_slice_size * desc.depth * layer;
            const uint8_t* src = static_cast<const uint8_t*>(sub_resouce.data);

            for (size_t z = 0; z < desc.depth; ++z) {
                for (size_t y = 0; y < mip_info.num_block_y; ++y)
                    std::memcpy(dst + dst_slice_size * z + dst_row_size * y,
                        src + src_slice_size * z + src_row_size * y, dst_row_size );
            }

        }

        // Fill copy structs
        VkBufferImageCopy copy;
        copy.bufferOffset = mip_info.offset;
        copy.bufferRowLength = mip_info.row_length;
        copy.bufferImageHeight = mip_info.image_height;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = level;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = desc.arraySize;
        copy.imageOffset = {0, 0, 0};
        copy.imageExtent = {mip_info.width, mip_info.height, mip_info.depth};
        copys.push_back(copy);

    }
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

    // Static data copy & layout transition
    if (init_data != nullptr && (desc.usageBits & IMAGE_USAGE_SAMPLING_BIT)) {
        CORE_DEBUG_ASSERT(desc.type == ImageType::TYPE_2D)

        // Prepare the mipmap infomation for copying
        TextureFormatLayout layout;
        layout.SetUp2D(desc.format, desc.width, desc.height, desc.arraySize, desc.generateMipMaps? 0 : desc.mipLevels);

        // Allocate a copy cmd with staging buffer
        Device_Vulkan::CopyCmdAllocator::CopyCmd copyCmd = device_->copyAllocator.allocate(layout.GetRequiredSize());

        // Fill staging buffer and copy structs
        std::vector<VkBufferImageCopy> copys;
        PrepareCopy(desc, layout, init_data, copyCmd.stageBuffer, copys);

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
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(copyCmd.cmdBuffer, &dependencyInfo);

        // Copy to image
        vkCmdCopyBufferToImage(copyCmd.cmdBuffer, ToInternal(copyCmd.stageBuffer.get()).handle_, handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copys.size(), copys.data());

        // Transit image layout to required init layout
        std::swap(barrier.srcStageMask, barrier.dstStageMask);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = ConvertImageLayout(desc_.initialLayout);
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = ParseImageLayoutToMemoryAccess(desc.initialLayout);
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(copyCmd.cmdBuffer, &dependencyInfo);

        // submit and block cpu
        device_->copyAllocator.submit(copyCmd);
    }
    else if (desc.initialLayout != ImageLayout::UNDEFINED) {    // Transit layout to required init layout
        Device_Vulkan::CopyCmdAllocator::CopyCmd transitCmd = device_->copyAllocator.allocate(0);

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
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(transitCmd.cmdBuffer, &dependencyInfo);

        device_->copyAllocator.submit(transitCmd);
    }
    
    CORE_LOGD("Vulkan image created")
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

    CORE_LOGD("Vulkan image destroyed")

}

Sampler_Vulkan::Sampler_Vulkan(Device_Vulkan* device, const SamplerDesc& desc)
    : device_(device)
{
    auto convert_sampler_filter = [](SamplerFilter filter) {
        switch (filter) {
        case SamplerFilter::NEAREST:
            return VK_FILTER_NEAREST;
        case SamplerFilter::LINEAR:
            return VK_FILTER_LINEAR;
        default:
            return VK_FILTER_MAX_ENUM;
        }
    };

    auto convert_sampler_address_mode = [](SamplerAddressMode mode) {
        switch (mode) {
        case SamplerAddressMode::REPEAT:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerAddressMode::MIRRORED_REPEAT:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case SamplerAddressMode::CLAMPED_TO_EDGE:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        default:
            return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
        }
    };

    // Sampler create info
	VkSamplerCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter = convert_sampler_filter(desc.minFilter);
	info.minFilter = convert_sampler_filter(desc.magFliter);
	info.addressModeU = convert_sampler_address_mode(desc.addressModeU);
	info.addressModeV = convert_sampler_address_mode(desc.addressModeV);
	info.addressModeW = convert_sampler_address_mode(desc.addressModeW);
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable = VK_FALSE;
	info.compareOp = VK_COMPARE_OP_ALWAYS;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.mipLodBias = 0.f;
	info.minLod = 0.f;
	info.maxLod = 1;

    if (desc.enableAnisotropy) {
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = device_->context->properties2.properties.limits.maxSamplerAnisotropy;
    }
    else {
        info.anisotropyEnable = VK_FALSE;
    }

    VK_CHECK(vkCreateSampler(device_->vkDevice, &info, nullptr, &handle_))

    CORE_LOGD("Vulkan sampler Created")
}

Sampler_Vulkan::~Sampler_Vulkan()
{
    if (handle_) {
        device_->GetCurrentFrame().garbageSamplers.push_back(handle_);
    }
    CORE_LOGD("Vulkan sampler destroyed")
}

}