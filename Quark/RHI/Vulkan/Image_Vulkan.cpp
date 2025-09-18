#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/Image_Vulkan.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi {

static inline VkImageAspectFlags FormatToAspectMask(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return 0;

    case VK_FORMAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static inline ImageViewType GetImageViewType(const ImageDesc& create_info, const ImageViewDesc* view)
{
    unsigned layers = view ? view->layerCount : create_info.arraySize;
    unsigned base_layer = view ? view->baseLayer : 0;

    if (layers == REMAINING_ARRAY_LAYERS)
        layers = create_info.arraySize - base_layer;

    QK_CORE_ASSERT(create_info.width >= 1);
    QK_CORE_ASSERT(create_info.height >= 1);
    switch (create_info.type)
    {
    case ImageType::TYPE_2D:
        QK_CORE_ASSERT(create_info.depth == 1);
        if (layers > 1)
            return ImageViewType::TYPE_2D_ARRAY;
        else
            return ImageViewType::TYPE_2D;
    case ImageType::TYPE_CUBE:
        QK_CORE_ASSERT(create_info.depth == 1);
        QK_CORE_ASSERT(create_info.width == create_info.height);
        if (layers % 6 == 0)
        {
            if (layers > 6)
                return ImageViewType::TYPE_CUBE_ARRAY;
            else
                return ImageViewType::TYPE_CUBE;
        }
        else
        {
            if (layers > 1)
                return ImageViewType::TYPE_2D_ARRAY;
            else
                return ImageViewType::TYPE_2D;
        }
    case ImageType::TYPE_3D:
        QK_CORE_ASSERT(create_info.depth >= 1);
        return ImageViewType::TYPE_3D;

    default:
        QK_CORE_ASSERT(0);
        return ImageViewType::TYPE_MAX_ENUM;
    }
}

static constexpr inline VkImageViewType ConvertImageViewType(ImageViewType type)
{
    switch (type)
    {
    case ImageViewType::TYPE_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case ImageViewType::TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case ImageViewType::TYPE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case ImageViewType::TYPE_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case ImageViewType::TYPE_1D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case ImageViewType::TYPE_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case ImageViewType::TYPE_CUBE_ARRAY:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default:
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

static constexpr VkAccessFlags2 ParseImageLayoutToMemoryAccess(ImageLayout layout)
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

static VkImageUsageFlags ConvertImageUsageFlags(uint32_t usageBits)
{
    VkImageUsageFlags ret = 0;

    if (usageBits & IMAGE_USAGE_SAMPLING_BIT) {
        ret |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (usageBits & IMAGE_USAGE_STORAGE_BIT) {
        ret |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (usageBits & IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (usageBits & IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        ret |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (usageBits & IMAGE_USAGE_CAN_COPY_FROM_BIT) {
        ret |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (usageBits & IMAGE_USAGE_CAN_COPY_TO_BIT) {
        ret |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (usageBits & IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        ret |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    return ret;
}

Image_Vulkan::Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc)
    : Image(desc), Cookie(device), m_device(device), 
    m_allocation(VK_NULL_HANDLE), m_handle(VK_NULL_HANDLE), m_isSwapChainImage(true)
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
            size_t src_row_size = sub_resouce.rowPitch? sub_resouce.rowPitch : mip_info.row_length;
            size_t src_slice_size = sub_resouce.slicePitch ? sub_resouce.rowPitch : mip_info.slice_pitch;;

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
    
    auto& vulkan_ctx = m_device->GetVulkanContext();
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
        vulkan_ctx.extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);
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

        vulkan_ctx.extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);

        // Blit image
        vkCmdBlitImage(cmd, m_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // Transit the dst mip level to transfer src layout
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        vulkan_ctx.extendFunction.pVkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);
    }
}

Image_Vulkan::Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc, const ImageInitData* init_data)
    : Image(desc), m_device(device), Cookie(device)
{
    QK_CORE_ASSERT(device != nullptr)
    auto& vk_context = m_device->GetVulkanContext();
    auto vk_device = m_device->vkDevice;

    // Default values
    m_handle = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
    m_isSwapChainImage = false;

    // Image create info
	VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.format = ConvertDataFormat(desc.format);
    create_info.extent.width = desc.width;
    create_info.extent.height = desc.height;
    create_info.extent.depth = desc.depth;
    create_info.mipLevels = desc.mipLevels? desc.mipLevels : TextureFormatLayout::GeneratedMipCount(desc.width, desc.height, desc.depth);
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

    if (vk_context.uniqueQueueFamilies.size() > 1) {
        create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = (uint32_t)vk_context.uniqueQueueFamilies.size();
        create_info.pQueueFamilyIndices = vk_context.uniqueQueueFamilies.data();
    }

    create_info.usage = ConvertImageUsageFlags(desc.usageBits);

    // Allocation info
    VmaAllocationCreateInfo allocCreateInfo = {}; // TODO: allocate a pool for small allocation
    allocCreateInfo.flags = 0;
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    VK_CHECK(vmaCreateImage(m_device->vmaAllocator, &create_info, &allocCreateInfo, &m_handle, &m_allocation, nullptr))

    // Image view create info
    VkImageViewCreateInfo image_view_createinfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    image_view_createinfo.image = m_handle;
    image_view_createinfo.format = ConvertDataFormat(desc.format);
    image_view_createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // TODO: Make components configurable
    image_view_createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_createinfo.subresourceRange.levelCount = create_info.mipLevels;
    image_view_createinfo.subresourceRange.layerCount = create_info.arrayLayers;
    image_view_createinfo.subresourceRange.baseMipLevel = 0;
    image_view_createinfo.subresourceRange.baseArrayLayer = 0;
    switch (desc.type) {
    case ImageType::TYPE_2D:
        image_view_createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case ImageType::TYPE_3D:
        image_view_createinfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    case ImageType::TYPE_CUBE:
    {
        image_view_createinfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    }
    }

    if (IsFormatSupportDepth(desc.format)) {
        image_view_createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        image_view_createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageView view;
    VK_CHECK(vkCreateImageView(vk_device, &image_view_createinfo, nullptr, &view));

    // kinda ugly 
    ImageViewDesc view_desc;
    view_desc.image = this;
    view_desc.format = desc.format;
    view_desc.baseLayer = 0;
    view_desc.layerCount = create_info.arrayLayers;
    view_desc.baseLevel = 0;
    view_desc.levelCount = create_info.mipLevels;
    view_desc.aspect = FormatToImageAspect(desc.format);
    view_desc.viewType = GetImageViewType(desc, nullptr);
    m_default_view = CreateRef<ImageView_Vulkan>(device, view, view_desc);

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
        vk_context.extendFunction.pVkCmdPipelineBarrier2KHR(copyCmd.transferCmdBuffer, &dependencyInfo);

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

            vk_context.extendFunction.pVkCmdPipelineBarrier2KHR(copyCmd.transitionCmdBuffer, &dependencyInfo);
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
        vk_context.extendFunction.pVkCmdPipelineBarrier2KHR(transitCmd.transitionCmdBuffer, &dependencyInfo);

        m_device->copyAllocator.submit(transitCmd);
    }
    
}

Image_Vulkan::~Image_Vulkan()
{
    if (m_isSwapChainImage)
        return;

    m_default_view.reset();
    if (m_internal_sync)
    {
        m_device->DestroyImageNoLock(m_handle, m_allocation);
        //m_device->DestroyImageViewNoLock(m_view);
    }
    else
    {
        m_device->DestroyImage(m_handle, m_allocation);
        // m_device->DestroyImageView(m_view);
    }

    QK_CORE_LOGT_TAG("RHI", "Vulkan image destroyed");

}

const ImageView& Image_Vulkan::GetDefaultView() const
{
    QK_CORE_ASSERT(m_default_view);
    return *m_default_view;
}

ImageView& Image_Vulkan::GetDefaultView()
{
    QK_CORE_ASSERT(m_default_view);
    return *m_default_view;
}

Sampler_Vulkan::Sampler_Vulkan(Device_Vulkan* device, const SamplerDesc& desc)
    : Sampler(desc), m_device(device)
{
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
	info.magFilter = ConvertSamplerFilter(desc.minFilter);
	info.minFilter = ConvertSamplerFilter(desc.magFliter);
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
        info.maxAnisotropy = m_device->GetVulkanContext().gpu_properties2.properties.limits.maxSamplerAnisotropy;
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

ImageView_Vulkan::ImageView_Vulkan(Device_Vulkan* device, const ImageViewDesc& desc)
    :ImageView(desc), Cookie(device), m_device(device)
{
    const ImageDesc& image_desc = desc.image->GetDesc();

    if ((image_desc.usageBits & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) == 0)
    {
        QK_CORE_LOGE_TAG("RHI","Cannot create image view unless certain usage flags are present.\n");
        return;
    }

    VkFormat format = (desc.format != DataFormat::UNDEFINED) ? ConvertDataFormat(desc.format) : ConvertDataFormat(image_desc.format);
    VkImageUsageFlags usage = ConvertImageUsageFlags(image_desc.usageBits);

    VkFormatProperties3 props3 = { VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3 };
    m_device->GetFormatProperties(format, &props3);
    if ((props3.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0)
        usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((props3.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0)
        usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;

    VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_info.image = ToInternal(desc.image).GetHandle();
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // TODO: Make components configurable
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = desc.aspect ?
        ConvertImageAspect(desc.aspect) : FormatToAspectMask(format);
    view_info.subresourceRange.baseMipLevel = desc.baseLevel;
    view_info.subresourceRange.baseArrayLayer = desc.baseLayer;
    view_info.subresourceRange.levelCount = desc.levelCount;
    view_info.subresourceRange.layerCount = desc.layerCount;


    if (desc.viewType == ImageViewType::TYPE_MAX_ENUM)
        view_info.viewType = ConvertImageViewType(GetImageViewType(image_desc, &desc));
    else
        view_info.viewType = ConvertImageViewType(desc.viewType);

    uint32_t num_levels;
    if (view_info.subresourceRange.levelCount == REMAINING_MIP_LEVELS)
        num_levels = desc.image->GetDesc().mipLevels - view_info.subresourceRange.baseMipLevel;
    else
        num_levels = view_info.subresourceRange.levelCount;

    uint32_t num_layers;
    if (view_info.subresourceRange.layerCount == REMAINING_ARRAY_LAYERS)
        num_layers = desc.image->GetDesc() .arraySize- view_info.subresourceRange.baseArrayLayer;
    else
        num_layers = view_info.subresourceRange.layerCount;

    view_info.subresourceRange.levelCount = num_levels;
    view_info.subresourceRange.layerCount = num_layers;

    CreateDefalutView(view_info);

}

ImageView_Vulkan::ImageView_Vulkan(Device_Vulkan* device, VkImageView default_view, const ImageViewDesc& desc)
    :ImageView(desc), Cookie(device), m_device(device), m_view(default_view)
{

}

ImageView_Vulkan::~ImageView_Vulkan()
{
    if (ToInternal(m_desc.image).IsSwapChainImage())
        return;

    FreeView(m_view);
    // FreeView(m_depth_view);
}

const VkImageView ImageView_Vulkan::GetRTView(uint32_t layer) const 
{
    QK_CORE_ASSERT(layer < m_desc.layerCount);

    if (m_render_target_views.empty())
        return m_view;
    else
    {
        QK_CORE_ASSERT(layer < m_render_target_views.size());
        return m_render_target_views[layer];
    }
}

const VkImageView ImageView_Vulkan::GetMipView(uint32_t level) const 
{
    QK_CORE_ASSERT(level < m_desc.layerCount);

    if (m_mip_views.empty())
        return m_view;
    else
    {
        QK_CORE_ASSERT(level < m_mip_views.size());
        return m_mip_views[level];
    }
}

bool ImageView_Vulkan::CreateDefalutView(const VkImageViewCreateInfo& info)
{
    VkDevice device = m_device->vkDevice;

    VkResult result = vkCreateImageView(device, &info, nullptr, &m_view);
    if (result != VK_SUCCESS)
    {
        VULKAN_ERROR(result);
        return false;
    }

    return true;
}

bool ImageView_Vulkan::CreateRTViews(const ImageDesc& image_desc, const VkImageViewCreateInfo& info)
{
    if (info.viewType == VK_IMAGE_VIEW_TYPE_3D)
        return true;

    m_render_target_views.reserve(info.subresourceRange.layerCount);

    // If we have a render target, and non-trivial case (layers = 1, levels = 1),
    // create an array of render targets which correspond to each layer (mip 0).
    if ((image_desc.usageBits & (IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0 &&
        ((info.subresourceRange.levelCount > 1) && (info.subresourceRange.layerCount > 1)))
    {
        auto view_info = info;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        for (uint32_t layer = 0; layer < info.subresourceRange.layerCount; layer++)
        {
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.layerCount = 1;
            view_info.subresourceRange.baseArrayLayer = info.subresourceRange.baseArrayLayer + layer;

            VkImageView rt_view;
            VkResult res = vkCreateImageView(m_device->vkDevice, &view_info, nullptr, &rt_view);
            if (res != VK_SUCCESS)
            {
                VULKAN_ERROR(res);
                return false;
            }
            m_render_target_views.push_back(rt_view);
        }
    }


    return true;
}

bool ImageView_Vulkan::CreateMipViews(const VkImageViewCreateInfo& info)
{
    QK_CORE_ASSERT(info.subresourceRange.levelCount != VK_REMAINING_MIP_LEVELS);

    if (info.subresourceRange.levelCount <= 1)
        return true;

    m_mip_views.reserve(info.subresourceRange.levelCount);

    auto view_info = info;
    for (uint32_t level = 0; level < info.subresourceRange.levelCount; level++)
    {
        view_info.subresourceRange.baseMipLevel = level;
        view_info.subresourceRange.levelCount = 1;
        
        VkImageView mip_view;
        VkResult res = vkCreateImageView(m_device->vkDevice, &view_info, nullptr, &mip_view);
        if (res != VK_SUCCESS)
        {
            VULKAN_ERROR(res);
            return false;
        }

        m_mip_views.push_back(mip_view);
    }

    return true;
}

bool ImageView_Vulkan::CreateAlternateViews(const ImageDesc& image_desc, const VkImageViewCreateInfo& info)
{
    if (info.viewType == VK_IMAGE_VIEW_TYPE_CUBE ||
        info.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY ||
        info.viewType == VK_IMAGE_VIEW_TYPE_3D)
    {
        return true;
    }

    VkDevice device = m_device->vkDevice;

    if (info.subresourceRange.aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
    {
        if ((image_desc.usageBits & ~IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
        {
            auto view_info = info;

            VkResult res;
            // We need this to be able to sample the texture, or otherwise use it as a non-pure DS attachment.
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            res = vkCreateImageView(device, &view_info, nullptr, &m_depth_view);
            if (res != VK_SUCCESS)
            {
                VULKAN_ERROR(res);
                return false;
            }

            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            res = vkCreateImageView(device, &view_info, nullptr, &m_stencil_view);
            if (res != VK_SUCCESS)
            {
                VULKAN_ERROR(res);
                return false;
            }
        }

    }

    return true;
}

void ImageView_Vulkan::FreeView(VkImageView view)
{
    if (m_internal_sync)
    {
        if (view != VK_NULL_HANDLE)
            m_device->DestroyImageViewNoLock(view);
    }
    else
    {
        if (view != VK_NULL_HANDLE)
            m_device->DestroyImageView(view);
    }
}

}