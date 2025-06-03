#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/Image_Vulkan.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi {

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
        QK_CORE_ASSERT("Layout type not handled")
        return 0;
    }
    }
}

Image_Vulkan::Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc)
    : Image(desc), Cookie(device), m_device(device), 
    m_allocation(VK_NULL_HANDLE), m_handle(VK_NULL_HANDLE), m_view(VK_NULL_HANDLE), m_isSwapChainImage(true)
{

}

void Image_Vulkan::PrepareCopy(const ImageDesc& desc, const TextureFormatLayout& layout, const ImageInitData* init_data, Ref<Buffer> stage_buffer, std::vector<VkBufferImageCopy>& copys)
{
    QK_CORE_ASSERT(copys.empty())
    QK_CORE_ASSERT(stage_buffer->GetDesc().size >= layout.GetRequiredSize())

    // Get mapped data ptr
    void* mapped = stage_buffer->GetMappedDataPtr();

    size_t index = 0;
    // Loop per mipmap level to copy data into staging buffer
    // and remove padding if necessary
    for (uint32_t level = 0; level < layout.GetMipLevels(); level++) {
        const auto& mip_info = layout.GetMipInfo(level);
        size_t dst_row_pitch = mip_info.num_block_x * layout.GetBlockStride();
        size_t dst_slice_pitch = dst_row_pitch * mip_info.num_block_y;

        for (uint32_t layer = 0; layer < desc.arraySize; layer++, index++) {
            const ImageInitData& sub_resouce = init_data[index];
            size_t src_row_size = sub_resouce.rowPitch;
            size_t src_slice_size = sub_resouce.slicePitch;

            uint8_t* dst = static_cast<uint8_t*>(mapped) + mip_info.offset + dst_slice_pitch * desc.depth * layer;
            const uint8_t* src = static_cast<const uint8_t*>(sub_resouce.data);

            // Remove padding
            for (uint32_t z = 0; z < desc.depth; ++z) {
                for (uint32_t y = 0; y < mip_info.num_block_y; ++y)
                    std::memcpy(dst + dst_slice_pitch * z + dst_row_pitch * y,
                        src + src_slice_size * z + src_row_size * y, dst_row_pitch );
            }

        }

        // Fill copy structs
        VkBufferImageCopy copy;
        copy.bufferOffset = mip_info.offset;
        copy.bufferRowLength = 0;   // padding has been removed in the above loop
        copy.bufferImageHeight = 0;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = level;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = desc.arraySize;
        copy.imageOffset = {0, 0, 0};
        copy.imageExtent = {mip_info.width, mip_info.height, mip_info.depth};
        copys.push_back(copy);

    }
}

void Image_Vulkan::GenerateMipMap(const ImageDesc& desc, VkCommandBuffer cmd)
{

    // Generate mipmap layout
    TextureFormatLayout layout;
    layout.SetUp2D(desc.format, desc.width, desc.height, desc.arraySize, 0);

    // Transit the base mip level to transfer src layout
    {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.image = m_handle;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer =0;
        barrier.subresourceRange.layerCount = desc.arraySize;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        m_device->vkContext->extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);
    }

    // Generate mipmaps
    for (uint32_t level = 1; level < layout.GetMipLevels(); ++level) {
        VkImageBlit blit = {};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.layerCount = desc.arraySize;
        blit.srcSubresource.mipLevel = level - 1;
        blit.srcOffsets[1].x = layout.GetMipInfo(level - 1).width;
        blit.srcOffsets[1].y = layout.GetMipInfo(level - 1).height;
        blit.srcOffsets[1].z = desc.depth;

        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.layerCount = desc.arraySize;
        blit.dstSubresource.mipLevel = level;
        blit.dstOffsets[1].x = layout.GetMipInfo(level).width;
        blit.dstOffsets[1].y = layout.GetMipInfo(level).height;
        blit.dstOffsets[1].z = desc.depth;

        // Transit the dst mip level to transfer dst layout
        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.image = m_handle;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = level;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = desc.arraySize;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        m_device->vkContext->extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);

        // Blit image
        vkCmdBlitImage(cmd, m_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // Transit the dst mip level to transfer src layout
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        m_device->vkContext->extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);
    }
}

Image_Vulkan::Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc, const ImageInitData* init_data)
    : Image(desc), m_device(device), Cookie(device)
{
    QK_CORE_ASSERT(device != nullptr)
    auto& vk_context = m_device->vkContext;
    auto vk_device = m_device->vkDevice;

    // Default values
    m_handle = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
    m_view = VK_NULL_HANDLE;
    m_isSwapChainImage = false;

    // Image create info
	VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.format = ConvertDataFormat(desc.format);
    create_info.extent.width = desc.width;
    create_info.extent.height = desc.height;
    create_info.extent.depth = desc.depth;
    create_info.mipLevels = desc.generateMipMaps? TextureFormatLayout::GeneratedMipCount(desc.width, desc.height, desc.depth) : desc.mipLevels;
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
        QK_CORE_VERIFY(desc.arraySize == 6) // only support cube texture with 6 images now
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    }
    }

    if (vk_context->uniqueQueueFamilies.size() > 1) {
        create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = (uint32_t)vk_context->uniqueQueueFamilies.size();
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
    VK_CHECK(vmaCreateImage(m_device->vmaAllocator, &create_info, &allocCreateInfo, &m_handle, &m_allocation, nullptr))

        // Image view create info
    VkImageViewCreateInfo image_m_Viewcreate_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    image_m_Viewcreate_info.image = m_handle;
    image_m_Viewcreate_info.format = ConvertDataFormat(desc.format);
    image_m_Viewcreate_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // TODO: Make components configurable
    image_m_Viewcreate_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_m_Viewcreate_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_m_Viewcreate_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_m_Viewcreate_info.subresourceRange.levelCount = create_info.mipLevels;
    image_m_Viewcreate_info.subresourceRange.layerCount = create_info.arrayLayers;
    image_m_Viewcreate_info.subresourceRange.baseMipLevel = 0;
    image_m_Viewcreate_info.subresourceRange.baseArrayLayer = 0;
    switch (desc.type) {
    case ImageType::TYPE_2D:
        image_m_Viewcreate_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case ImageType::TYPE_3D:
        image_m_Viewcreate_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    case ImageType::TYPE_CUBE:
    {
        image_m_Viewcreate_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    }
    }
    
    if (IsFormatSupportDepth(desc.format)) {
        image_m_Viewcreate_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        image_m_Viewcreate_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VK_CHECK(vkCreateImageView(vk_device, &image_m_Viewcreate_info, nullptr, &m_view))

    // Static data copy & layout transition
    if (init_data != nullptr && (desc.usageBits & IMAGE_USAGE_SAMPLING_BIT)) {
        QK_CORE_ASSERT(desc.type == ImageType::TYPE_2D || desc.type == ImageType::TYPE_CUBE)

        // Prepare the mipmap infomation for copying
        TextureFormatLayout layout;
        layout.SetUp2D(desc.format, desc.width, desc.height, desc.arraySize, desc.generateMipMaps? 1 : desc.mipLevels);

        // Allocate a copy cmd with staging buffer
        CopyCmdAllocator::CopyCmd copyCmd = m_device->copyAllocator.allocate(layout.GetRequiredSize());

        // Fill staging buffer and copy structs
        std::vector<VkBufferImageCopy> copys;
        PrepareCopy(desc, layout, init_data, copyCmd.stageBuffer, copys);

        // Transit image to transfer dst format
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.image = m_handle;
        barrier.oldLayout = create_info.initialLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = layout.GetMipLevels();
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = create_info.arrayLayers;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(copyCmd.transferCmdBuffer, &dependencyInfo);

        // Copy to image
        vkCmdCopyBufferToImage(copyCmd.transferCmdBuffer, ToInternal(copyCmd.stageBuffer.get()).GetHandle(), m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copys.size(), copys.data());
        
        // Generate mipmaps? //TODO: change to use graphic queue to generate mipmaps
        if (desc.generateMipMaps) 
            GenerateMipMap(desc, copyCmd.transitionCmdBuffer); // After generating, image is in the layout of VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        // Transit image layout to required init layout
        if (desc.initialLayout != ImageLayout::UNDEFINED)
        {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; //TODO: Parse to correct stage
            barrier.dstAccessMask = ParseImageLayoutToMemoryAccess(desc.initialLayout);
            barrier.subresourceRange.levelCount = create_info.mipLevels;
            barrier.oldLayout = desc.generateMipMaps ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = ConvertImageLayout(desc.initialLayout);

            vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(copyCmd.transitionCmdBuffer, &dependencyInfo);
        }

        // submit and block cpu
        m_device->copyAllocator.submit(copyCmd);
    }
    else if (desc.initialLayout != ImageLayout::UNDEFINED) {    // Transit layout to required init layout
        CopyCmdAllocator::CopyCmd transitCmd = m_device->copyAllocator.allocate(0);

        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = m_handle;
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
        if (IsFormatSupportDepth(desc.format))
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        vk_context->extendFunction.pVkCmdPipelineBarrier2KHR(transitCmd.transitionCmdBuffer, &dependencyInfo);

        m_device->copyAllocator.submit(transitCmd);
    }
    
}

Image_Vulkan::~Image_Vulkan()
{
    if (m_isSwapChainImage)
        return;

    //auto& frame = m_device->GetCurrentFrame();
    //if (m_handle != VK_NULL_HANDLE) {
    //    frame.garbageImages.push_back(std::make_pair(m_handle, m_allocation));
    //}

    //if (m_view) {
    //    frame.grabageViews.push_back(m_view);
    //}

    if (m_internal_sync)
    {
        m_device->DestroyImageNoLock(m_handle, m_allocation);
        m_device->DestroyImageViewNoLock(m_view);
    }
    else
    {
        m_device->DestroyImage(m_handle, m_allocation);
        m_device->DestroyImageView(m_view);
    }

    QK_CORE_LOGT_TAG("RHI", "Vulkan image destroyed");

}

Sampler_Vulkan::Sampler_Vulkan(Device_Vulkan* device, const SamplerDesc& desc)
    : Sampler(desc), m_device(device)
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
        info.maxAnisotropy = m_device->vkContext->properties2.properties.limits.maxSamplerAnisotropy;
    }
    else {
        info.anisotropyEnable = VK_FALSE;
    }

    VK_CHECK(vkCreateSampler(m_device->vkDevice, &info, nullptr, &m_handle))

}

Sampler_Vulkan::~Sampler_Vulkan()
{
    if (m_handle) {
        m_device->GetCurrentFrame().garbage_samplers.push_back(m_handle);
    }
    QK_CORE_LOGT_TAG("RHI", "Vulkan sampler destroyed");
}

}