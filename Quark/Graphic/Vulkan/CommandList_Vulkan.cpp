#include "Quark/qkpch.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/Util/BitOperations.h"
#include "Quark/Graphic/Vulkan/CommandList_Vulkan.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#include "Quark/Graphic/Vulkan/PipeLine_Vulkan.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"

namespace quark::graphic {

constexpr VkImageAspectFlags _ConvertImageAspect(ImageAspect value)
{
    switch (value)
    {
    default:
    case graphic::ImageAspect::COLOR:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    case graphic::ImageAspect::DEPTH:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    case graphic::ImageAspect::STENCIL:
        return VK_IMAGE_ASPECT_STENCIL_BIT;
    case graphic::ImageAspect::LUMINANCE:
        return VK_IMAGE_ASPECT_PLANE_0_BIT;
    case graphic::ImageAspect::CHROMINANCE:
        return VK_IMAGE_ASPECT_PLANE_1_BIT;
    }
}

CommandList_Vulkan::CommandList_Vulkan(Device_Vulkan* device, QueueType type)
    : CommandList(type), m_GraphicDevice(device)
{
    QK_CORE_ASSERT(m_GraphicDevice != nullptr)
    auto& vulkan_context = m_GraphicDevice->vkContext;
    VkDevice vk_device = m_GraphicDevice->vkDevice;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // TODO: Support other queue types
    switch (m_QueueType) {
    case QUEUE_TYPE_GRAPHICS:
        poolInfo.queueFamilyIndex = vulkan_context->graphicQueueIndex;
        break;
    case QUEUE_TYPE_ASYNC_COMPUTE:
        poolInfo.queueFamilyIndex = vulkan_context->computeQueueIndex;
        break;
    case QUEUE_TYPE_ASYNC_TRANSFER:
        poolInfo.queueFamilyIndex = vulkan_context->transferQueueIndex;
        break;
    default:
        QK_CORE_VERIFY(0, "Queue Type not handled."); // queue type not handled
        break;
    }
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VK_CHECK(vkCreateCommandPool(vk_device, &poolInfo, nullptr, &m_CmdPool))

    // Allocate command buffer
    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandBufferCount = 1;
    commandBufferInfo.commandPool = m_CmdPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK(vkAllocateCommandBuffers(vk_device, &commandBufferInfo, &m_CmdBuffer))

    VkSemaphoreCreateInfo semCreateInfo = {};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(vk_device, &semCreateInfo, nullptr, &m_CmdCompleteSemaphore))
}

void CommandList_Vulkan::ResetAndBeginCmdBuffer()
{
    // Reset status
    vkResetCommandPool(m_GraphicDevice->vkDevice, m_CmdPool, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional
    VK_CHECK(vkBeginCommandBuffer(m_CmdBuffer, &beginInfo))

    m_WaitForSwapchainImage = false;
    m_SwapChainWaitStages = 0;
    m_ImageBarriers.clear();
    m_BufferBarriers.clear();
    m_MemoryBarriers.clear();
    state = CommandListState::IN_RECORDING;

    m_CurrentPipeline = nullptr;
    ResetBindingState();
}

CommandList_Vulkan::~CommandList_Vulkan()
{
    vkDestroySemaphore(m_GraphicDevice->vkDevice, m_CmdCompleteSemaphore, nullptr);
    vkDestroyCommandPool(m_GraphicDevice->vkDevice, m_CmdPool, nullptr); 
}

void CommandList_Vulkan::PipeLineBarriers(const PipelineMemoryBarrier *memoryBarriers, u32 memoryBarriersCount, const PipelineImageBarrier *imageBarriers, u32 iamgeBarriersCount, const PipelineBufferBarrier *bufferBarriers, u32 bufferBarriersCount)
{
    QK_CORE_ASSERT(bufferBarriers == nullptr);   // do not support buffer barrier for now

    QK_CORE_ASSERT(m_MemoryBarriers.empty() && m_ImageBarriers.empty() && m_BufferBarriers.empty())

    // Memory Barriers
    for (size_t i = 0; i < memoryBarriersCount; ++i) {
        const PipelineMemoryBarrier& memory_barrier = memoryBarriers[i];
        VkMemoryBarrier2 vk_memory_barrier = {};
        vk_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		vk_memory_barrier.pNext = nullptr;
        vk_memory_barrier.srcStageMask = (VkPipelineStageFlagBits2)memory_barrier.srcStageBits;
        vk_memory_barrier.srcAccessMask = (VkAccessFlagBits2)memory_barrier.srcMemoryAccessBits;
        vk_memory_barrier.dstStageMask = (VkPipelineStageFlagBits2)memory_barrier.dstStageBits;
        vk_memory_barrier.dstAccessMask = (VkAccessFlagBits2)memory_barrier.dstMemoryAccessBits;
        m_MemoryBarriers.push_back(vk_memory_barrier);
    }

    // Image Barriers
    for (size_t i = 0; i < iamgeBarriersCount; ++i) {
        const PipelineImageBarrier& image_barrier = imageBarriers[i];
        auto& internal_image = ToInternal(image_barrier.image);
        auto& image_desc = image_barrier.image->GetDesc();
        auto image_format = image_desc.format;
        VkImageMemoryBarrier2 vk_image_barrier = {};
        vk_image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		vk_image_barrier.pNext = nullptr;
        vk_image_barrier.srcStageMask = (VkPipelineStageFlagBits2)image_barrier.srcStageBits;
        vk_image_barrier.srcAccessMask = (VkAccessFlagBits2)image_barrier.srcMemoryAccessBits;
        vk_image_barrier.dstStageMask = (VkPipelineStageFlagBits2)image_barrier.dstStageBits;
        vk_image_barrier.dstAccessMask = (VkAccessFlagBits2)image_barrier.dstMemoryAccessBits;
        vk_image_barrier.image = internal_image.GetHandle();
        vk_image_barrier.oldLayout = ConvertImageLayout(image_barrier.layoutBefore);
        vk_image_barrier.newLayout = ConvertImageLayout(image_barrier.layoutAfter);
        vk_image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        if (image_format == DataFormat::D32_SFLOAT ||
            image_format == DataFormat::D32_SFLOAT_S8_UINT ||
            image_format == DataFormat::D24_UNORM_S8_UINT) {
            vk_image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (image_format == DataFormat::D32_SFLOAT_S8_UINT || image_format == DataFormat::D24_UNORM_S8_UINT) {
                vk_image_barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        if (image_barrier.baseMipLevel != UINT32_MAX) {
            vk_image_barrier.subresourceRange.baseMipLevel = image_barrier.baseMipLevel;
            vk_image_barrier.subresourceRange.levelCount = 1;
        }
        else {
            vk_image_barrier.subresourceRange.baseMipLevel = 0;
            vk_image_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        }

        if (image_barrier.baseArrayLayer != UINT32_MAX) {
            vk_image_barrier.subresourceRange.baseArrayLayer = image_barrier.baseArrayLayer;
            vk_image_barrier.subresourceRange.layerCount = 1;
        }
        else {
            vk_image_barrier.subresourceRange.baseArrayLayer = 0;
            vk_image_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        }
        
        // we are using concurrent sharing mode
        vk_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        m_ImageBarriers.push_back(vk_image_barrier);
    }
    
    // TODO:Support buffer barrier

    if (!m_MemoryBarriers.empty() || !m_ImageBarriers.empty() || !m_BufferBarriers.empty()) {
        VkDependencyInfo dependency_info = {};
        dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.memoryBarrierCount = static_cast<uint32_t>(m_MemoryBarriers.size());
        dependency_info.pMemoryBarriers = m_MemoryBarriers.data();
        dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(m_BufferBarriers.size());
        dependency_info.pBufferMemoryBarriers = m_BufferBarriers.data();
        dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(m_ImageBarriers.size());
        dependency_info.pImageMemoryBarriers = m_ImageBarriers.data();

        m_GraphicDevice->vkContext->extendFunction.pVkCmdPipelineBarrier2KHR(m_CmdBuffer, &dependency_info);

        m_MemoryBarriers.clear();
        m_ImageBarriers.clear();
        m_BufferBarriers.clear();
    }
}

void CommandList_Vulkan::ResetBindingState()
{
    m_BindingState = {};

    for (int i = 0; i < DESCRIPTOR_SET_MAX_NUM; i++)
        m_CurrentSets[i] = VK_NULL_HANDLE;

    m_DirtySetMask = 0;
    m_DirtyVertexBufferMask = 0;
    m_DirtySetDynamicMask = 0;
}

void CommandList_Vulkan::BeginRenderPass(const RenderPassInfo2& renderPassInfo, const FrameBufferInfo& frameBufferInfo)
{
    QK_CORE_ASSERT(renderPassInfo.numColorAttachments < MAX_COLOR_ATTHACHEMNT_NUM)
        QK_CORE_ASSERT(frameBufferInfo.numResolveAttachments < renderPassInfo.numColorAttachments)

#if QK_DEBUG_BUILD
        if (state != CommandListState::IN_RECORDING)
            QK_CORE_LOGE_TAG("Graphic", "You must call BeginRenderPass() in recording state.");
#endif

    // Change state
    state = CommandListState::IN_RENDERPASS;
    m_CurrentRenderPassInfo2 = renderPassInfo;
    m_CurrentPipeline = nullptr;
    ResetBindingState();

    VkRenderingInfo rendering_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    rendering_info.layerCount = 1;
    rendering_info.renderArea.offset.x = 0;
    rendering_info.renderArea.offset.y = 0;
    rendering_info.renderArea.extent.width = 0;
    rendering_info.renderArea.extent.height = 0;
    rendering_info.colorAttachmentCount = renderPassInfo.numColorAttachments;

    VkRenderingAttachmentInfo color_attachments[MAX_COLOR_ATTHACHEMNT_NUM] = {};
    VkRenderingAttachmentInfo depth_attachment = {};

    auto convertLoadOp = [](FrameBufferInfo::AttachmentLoadOp op) -> VkAttachmentLoadOp {
        switch (op) {
        default:
        case FrameBufferInfo::AttachmentLoadOp::CLEAR:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case FrameBufferInfo::AttachmentLoadOp::LOAD:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case FrameBufferInfo::AttachmentLoadOp::DONTCARE:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        };

    auto convertStoreOp = [](FrameBufferInfo::AttachmentStoreOp op) ->VkAttachmentStoreOp {
        switch (op) {
        default:
        case FrameBufferInfo::AttachmentStoreOp::STORE:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case FrameBufferInfo::AttachmentStoreOp::DONTCARE:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        };

    // Color Attachments
    for (size_t i = 0; i < renderPassInfo.numColorAttachments; ++i) {
        const auto* image = frameBufferInfo.colorAttachments[i];
        const ImageDesc& image_desc = image->GetDesc();
        auto& internal_image = ToInternal(image);

        rendering_info.renderArea.extent.width = std::max(rendering_info.renderArea.extent.width, image_desc.width);
        rendering_info.renderArea.extent.height = std::max(rendering_info.renderArea.extent.height, image_desc.height);

        color_attachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachments[i].imageView = internal_image.GetView();
        color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].loadOp = convertLoadOp(frameBufferInfo.colorAttatchemtsLoadOp[i]);
        color_attachments[i].storeOp = convertStoreOp(frameBufferInfo.colorAttatchemtsStoreOp[i]);
        color_attachments[i].clearValue.color.float32[0] = frameBufferInfo.clearColors[i].color.float32[0];
        color_attachments[i].clearValue.color.float32[1] = frameBufferInfo.clearColors[i].color.float32[1];
        color_attachments[i].clearValue.color.float32[2] = frameBufferInfo.clearColors[i].color.float32[2];
        color_attachments[i].clearValue.color.float32[3] = frameBufferInfo.clearColors[i].color.float32[3];

        // internal swapchain image state tracking
        if (internal_image.IsSwapChainImage())
        {
            m_WaitForSwapchainImage = true;
            m_SwapChainWaitStages |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
    }

    // Resolve attatchments
    for (size_t i = 0; i < frameBufferInfo.numResolveAttachments; ++i) {
        color_attachments[i].resolveImageView = ToInternal(frameBufferInfo.resolveAttatchments[i]).GetView();
        color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    // Depth attatchment
    if (frameBufferInfo.depthAttachment != nullptr) {
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = ToInternal(frameBufferInfo.depthAttachment).GetView();
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = convertLoadOp(frameBufferInfo.depthAttachmentLoadOp);
        depth_attachment.storeOp = convertStoreOp(frameBufferInfo.depthAttachmentStoreOp);
        depth_attachment.clearValue.depthStencil.depth = frameBufferInfo.clearDepthStencil.depth_stencil.depth;

    }

    rendering_info.pColorAttachments = renderPassInfo.numColorAttachments > 0 ? color_attachments : nullptr;
    rendering_info.pDepthAttachment = frameBufferInfo.depthAttachment ? &depth_attachment : nullptr;
    //TODO: Support stencil test
    rendering_info.pStencilAttachment = nullptr;
    rendering_info.pNext = nullptr;

    m_GraphicDevice->vkContext->extendFunction.pVkCmdBeginRenderingKHR(m_CmdBuffer, &rendering_info);
}

//void CommandList_Vulkan::BeginRenderPass(const RenderPassInfo &info)
//{
//    QK_CORE_ASSERT(info.numColorAttachments < MAX_COLOR_ATTHACHEMNT_NUM)
//    QK_CORE_ASSERT(info.numResolveAttachments < MAX_COLOR_ATTHACHEMNT_NUM)
//
//#if QK_DEBUG_BUILD
//    if (state != CommandListState::IN_RECORDING) {
//        CORE_LOGE("You must call BeginRenderPass() in recording state.")
//    }
//#endif
//
//    // Change state
//    state = CommandListState::IN_RENDERPASS;
//    //m_CurrentRenderPassInfo = &info;
//    m_CurrentPipeline = nullptr;
//    ResetBindingState();
//
//    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
//    rendering_info.layerCount = 1;
//    rendering_info.renderArea.offset.x = 0;
//    rendering_info.renderArea.offset.y = 0;
//    rendering_info.renderArea.extent.width = 0;
//    rendering_info.renderArea.extent.height = 0;
//    rendering_info.colorAttachmentCount = info.numColorAttachments;
//
//    VkRenderingAttachmentInfo color_attachments[MAX_COLOR_ATTHACHEMNT_NUM] = {};
//    VkRenderingAttachmentInfo depth_attachment = {};
//
//    auto convertLoadOp = [](RenderPassInfo::AttachmentLoadOp op) -> VkAttachmentLoadOp {
//        switch (op) {
//        default:
//        case RenderPassInfo::AttachmentLoadOp::CLEAR:
//            return VK_ATTACHMENT_LOAD_OP_CLEAR;
//        case RenderPassInfo::AttachmentLoadOp::LOAD:
//            return VK_ATTACHMENT_LOAD_OP_LOAD;
//        case RenderPassInfo::AttachmentLoadOp::DONTCARE:
//            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        }
//    };
//
//    auto convertStoreOp = [](RenderPassInfo::AttachmentStoreOp op) ->VkAttachmentStoreOp {
//        switch (op) {
//        default:
//        case RenderPassInfo::AttachmentStoreOp::STORE:
//            return VK_ATTACHMENT_STORE_OP_STORE;
//        case RenderPassInfo::AttachmentStoreOp::DONTCARE:
//            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
//        }
//    };
//
//    // Color Attachments
//    for (size_t i = 0; i < info.numColorAttachments; ++i) {
//        const auto image = info.colorAttachments[i];
//        const ImageDesc& image_desc = image->GetDesc();
//        auto& internal_image = ToInternal(image);
//
//        rendering_info.renderArea.extent.width = std::max(rendering_info.renderArea.extent.width, image_desc.width);
//		rendering_info.renderArea.extent.height = std::max(rendering_info.renderArea.extent.height, image_desc.height);
//
//        color_attachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
//        color_attachments[i].imageView = internal_image.GetView();
//        color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        color_attachments[i].loadOp = convertLoadOp(info.colorAttatchemtsLoadOp[i]);
//        color_attachments[i].storeOp = convertStoreOp(info.colorAttatchemtsStoreOp[i]);
//        color_attachments[i].clearValue.color.float32[0] = info.clearColors[i].color[0];
//        color_attachments[i].clearValue.color.float32[1] = info.clearColors[i].color[1];
//        color_attachments[i].clearValue.color.float32[2] = info.clearColors[i].color[2];
//        color_attachments[i].clearValue.color.float32[3] = info.clearColors[i].color[3];
//
//        // internal swapchain image state tracking
//        if (internal_image.IsSwapChainImage()) 
//        {
//            m_WaitForSwapchainImage = true;
//            m_SwapChainWaitStages |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
//        }
//    }
//
//    // Resolve attatchments
//    for (size_t i = 0; i < info.numResolveAttachments; ++i) {
//        color_attachments[i].resolveImageView = ToInternal(info.resolveAttatchments[i]).GetView();
//        color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        color_attachments[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
//    }
//
//    // Depth attatchment
//    if (info.depthAttachment != nullptr) {
//        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
//        depth_attachment.imageView = ToInternal(info.depthAttachment).GetView();
//        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        depth_attachment.loadOp = convertLoadOp(info.depthAttachmentLoadOp);
//        depth_attachment.storeOp = convertStoreOp(info.depthAttachmentStoreOp);
//        depth_attachment.clearValue.depthStencil.depth = info.ClearDepthStencil.depth_stencil.depth;
//
//    }
//
//    rendering_info.pColorAttachments = info.numColorAttachments > 0? color_attachments : nullptr;
//    rendering_info.pDepthAttachment = info.depthAttachment ? &depth_attachment : nullptr;
//    //TODO: Support stencil test
//    rendering_info.pStencilAttachment = nullptr;
//    rendering_info.pNext = nullptr;
//
//    m_GraphicDevice->vkContext->extendFunction.pVkCmdBeginRenderingKHR(m_CmdBuffer, &rendering_info);   
//}

void CommandList_Vulkan::EndRenderPass()
{   
#if QK_DEBUG_BUILD
    if (state != CommandListState::IN_RENDERPASS) {
        QK_CORE_LOGE_TAG("Graphic", "You must call BeginRenderPass() before calling EndRenderPass()");
    }
#endif
    // Set state back to in recording
    state = CommandListState::IN_RECORDING;
    m_GraphicDevice->vkContext->extendFunction.pVkCmdEndRenderingKHR(m_CmdBuffer);
}

void CommandList_Vulkan::PushConstant(const void *data, uint32_t offset, uint32_t size)
{
    QK_CORE_ASSERT(offset + size < PUSH_CONSTANT_DATA_SIZE)

#ifdef QK_DEBUG_BUILD
    if (m_CurrentPipeline == nullptr) 
    {
        QK_CORE_LOGE_TAG("Graphic", "You can not bind a push constant before binding a pipeline.");
        return;
    }
    if (m_CurrentPipeline->GetLayout()->combinedLayout.pushConstant.size == 0) 
    {
        QK_CORE_LOGE_TAG("Graphic", "Current pipeline's layout do not have a push constant");
        return;
    }
#endif

    auto& layout = *m_CurrentPipeline->GetLayout();
    vkCmdPushConstants(m_CmdBuffer,
        layout.handle,
        layout.combinedLayout.pushConstant.stageFlags,
        offset,
        size,
        data);
}

void CommandList_Vulkan::BindUniformBuffer(u32 set, u32 binding, const Buffer &buffer, u64 offset, u64 size)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (m_CurrentPipeline == nullptr) 
    {
        QK_CORE_LOGE_TAG("Graphic", "You must bind a pipeline before binding a uniform buffer.");
        return;
    }
    if ((buffer.GetDesc().usageBits & BUFFER_USAGE_UNIFORM_BUFFER_BIT) == 0) 
    {
        QK_CORE_LOGE_TAG("Graphic", "CommandList_Vulkan::BindUniformBuffer : The bounded buffer doesn't has BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        return;
    }
    if ((m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) == 0 ||
        (m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set].uniform_buffer_mask & (1u << binding))== 0) 
    {
        QK_CORE_LOGE_TAG("Graphic", "CommandList_Vulkan::BindUniformBuffer : Set: {}, binding: {} is not a uniforom buffer.", set, binding);
        return;
    }
#endif

    auto& internal_buffer = ToInternal(&buffer);
    auto& b = m_BindingState.descriptorBindings[set][binding];

    if (b.buffer.buffer == internal_buffer.GetHandle() && b.buffer.range == size) {
        if (b.dynamicOffset != offset) {
            m_DirtySetDynamicMask|= 1u << set;
            b.dynamicOffset = (uint32_t)offset;
        }
    }
    else {
        b.buffer = {internal_buffer.GetHandle(), 0, size};
        b.dynamicOffset = (uint32_t)offset;
        m_DirtySetMask |= 1u << set;
    }

}

void CommandList_Vulkan::BindStorageBuffer(u32 set, u32 binding, const Buffer &buffer, u64 offset, u64 size)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (m_CurrentPipeline == nullptr)
    {
        QK_CORE_LOGE_TAG("Graphic", "You must bind a pipeline before binding a storage buffer.");
        return;
    }
    if ((buffer.GetDesc().usageBits & BUFFER_USAGE_STORAGE_BUFFER_BIT) == 0)
    {
        QK_CORE_LOGE_TAG("Graphic", "CommandList_Vulkan::BindStorageBuffer : The bounded buffer doesn't has BUFFER_USAGE_STORAGE_BUFFER_BIT");
        return;
    }

    if ((m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) == 0 ||
        (m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set].storage_buffer_mask & (1u << binding)) == 0) 
    {
        QK_CORE_LOGE_TAG("Graphic", "CommandList_Vulkan::BindStorageBuffer : Set: {}, binding: {} is not a storage buffer.", set, binding);
        return;
    }
#endif

    auto& internal_buffer = ToInternal(&buffer);
    auto& b = m_BindingState.descriptorBindings[set][binding];

    if (b.buffer.buffer == internal_buffer.GetHandle() && b.buffer.range == size) 
        return;

    b.buffer = { internal_buffer.GetHandle(), offset, size };
    b.dynamicOffset = 0;
    m_DirtySetMask |= 1u << set;
}

void CommandList_Vulkan::BindImage(u32 set, u32 binding, const Image &image, ImageLayout layout)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (m_CurrentPipeline == nullptr) 
    {
        QK_CORE_LOGE_TAG("Graphic", "You must bind a pipeline before binding a image.");
        return;
    }
    if (!(image.GetDesc().usageBits & IMAGE_USAGE_SAMPLING_BIT) &&
        !(image.GetDesc().usageBits & IMAGE_USAGE_STORAGE_BIT)) 
    {
        QK_CORE_LOGE_TAG("Graphic", "Binded Image must with usage bits: IMAGE_USAGE_SAMPLING_BIT and IMAGE_USAGE_STORAGE_BIT");
        return;
    }
    if (layout != ImageLayout::SHADER_READ_ONLY_OPTIMAL && layout != ImageLayout::GENERAL) 
    {
        QK_CORE_LOGE_TAG("Graphic", "Bind image's layout can only be SHADER_READ_ONLY_OPTIMAL and GENERAL");
        return;
    }
#endif

    auto& internal_image = ToInternal(&image);
    auto& b = m_BindingState.descriptorBindings[set][binding];

    if (b.image.imageView == internal_image.GetView() && b.image.imageLayout == ConvertImageLayout(layout))
        return;

    b.image.imageView = internal_image.GetView();
    b.image.imageLayout = ConvertImageLayout(layout);
    m_DirtySetMask |= 1u << set;
    
}

void CommandList_Vulkan::BindPipeLine(const PipeLine &pipeline)
{
    auto& internal_pipeline = ToInternal(&pipeline);
    QK_CORE_ASSERT(internal_pipeline.GetHandle() != VK_NULL_HANDLE)

#ifdef QK_DEBUG_BUILD
    if (!m_CurrentRenderPassInfo2.IsValid()) 
    {
        QK_CORE_LOGE_TAG("Graphic", "BindPipeLine()::You must call BeginRenderPass() before binding a pipeline.");
        return;
    }

    const auto& render_pass_info = internal_pipeline.GetCompatableRenderPassInfo();
    if (render_pass_info.numColorAttachments != m_CurrentRenderPassInfo2.numColorAttachments) 
    {
        QK_CORE_LOGE_TAG("Graphic", "BindPipeLine()::The pipeline's color attachment number is not equal to the current render pass.");
        return;
    }

    if (render_pass_info.depthAttachmentFormat != m_CurrentRenderPassInfo2.depthAttachmentFormat)
    {
        QK_CORE_LOGE_TAG("Graphic", "BindPipeLine()::The pipeline's depth attachment is not equal to the current render pass.");
        return;
    }
#endif

    if (m_CurrentPipeline == &internal_pipeline)
        return;
    
    if (internal_pipeline.GetBindingPoint() == PipeLineBindingPoint::GRAPHIC)
        vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal_pipeline.GetHandle());
    else
        vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, internal_pipeline.GetHandle());

    // Change tracking state
    m_CurrentPipeline = &internal_pipeline;
    m_DirtySetMask = 0;
    m_DirtySetDynamicMask = 0;

    memset(m_BindingState.descriptorBindings, 0, sizeof(m_BindingState.descriptorBindings));

    for (int i = 0; i < DESCRIPTOR_SET_MAX_NUM; i++)
        m_CurrentSets[i] = VK_NULL_HANDLE;

}

void CommandList_Vulkan::BindSampler(u32 set, u32 binding, const Sampler& sampler)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (m_CurrentPipeline == nullptr) {
        QK_CORE_LOGE_TAG("Graphic", "You must bind a pipeline before binding a sampler.");
    }
#endif

    auto& internal_sampler = ToInternal(&sampler);
    auto& b = m_BindingState.descriptorBindings[set][binding];

    if (b.image.sampler == internal_sampler.GetHandle())
        return;

    b.image.sampler = internal_sampler.GetHandle();
    m_DirtySetMask |= 1u << set;
}

void CommandList_Vulkan::CopyImageToBuffer(const Buffer& buffer, const Image& image, uint64_t buffer_offset, const Offset3D& offset, const Extent3D& extent, uint32_t row_pitch, uint32_t slice_pitch, const ImageSubresourceRange& subresouce)
{
    auto& internal_buffer = ToInternal(&buffer);
	auto& internal_image = ToInternal(&image);

	VkBufferImageCopy copy_region = {};
	copy_region.bufferOffset = buffer_offset;
	copy_region.bufferRowLength = row_pitch;
	copy_region.bufferImageHeight = slice_pitch;
	copy_region.imageSubresource.aspectMask = _ConvertImageAspect(subresouce.aspect);
	copy_region.imageSubresource.baseArrayLayer = subresouce.baseArrayLayer;
	copy_region.imageSubresource.layerCount = subresouce.layerCount;
    copy_region.imageSubresource.mipLevel = subresouce.mipLevel;
	copy_region.imageOffset = { offset.x, offset.y, offset.z };
	copy_region.imageExtent = { extent.width, extent.height, extent.depth };

	vkCmdCopyImageToBuffer(m_CmdBuffer, internal_image.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, internal_buffer.GetHandle(), 1, &copy_region);
}

void CommandList_Vulkan::BindIndexBuffer(const Buffer &buffer, u64 offset, const IndexBufferFormat format)
{
    auto& internal_buffer = ToInternal(&buffer);
    QK_CORE_ASSERT(internal_buffer.GetHandle() != VK_NULL_HANDLE)
    QK_CORE_ASSERT((buffer.GetDesc().usageBits & BUFFER_USAGE_INDEX_BUFFER_BIT) != 0)
    
    auto& index_buffer_binding_state = m_BindingState.indexBufferBindingState;
    if (internal_buffer.GetHandle() == index_buffer_binding_state.buffer &&
        offset == index_buffer_binding_state.offset &&
        format == index_buffer_binding_state.format) {
        return;
    }

    index_buffer_binding_state.buffer = internal_buffer.GetHandle();
    index_buffer_binding_state.offset = offset;
    index_buffer_binding_state.format = format;

    vkCmdBindIndexBuffer(m_CmdBuffer, internal_buffer.GetHandle(), offset, (format == IndexBufferFormat::UINT16? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32));
}

void CommandList_Vulkan::BindVertexBuffer(u32 binding, const Buffer &buffer, u64 offset)
{
    auto& internal_buffer = ToInternal(&buffer);
    // TODO: add some state track for debuging here
    QK_CORE_ASSERT(binding < VERTEX_BUFFER_MAX_NUM)
    QK_CORE_ASSERT(buffer.GetDesc().usageBits & BUFFER_USAGE_VERTEX_BUFFER_BIT)

    auto& vertex_buffer_binding_state = m_BindingState.vertexBufferBindingState;
    if (vertex_buffer_binding_state.buffers[binding] == internal_buffer.GetHandle() &&
        vertex_buffer_binding_state.offsets[binding] == offset) 
    {
        return;
    }

    vertex_buffer_binding_state.buffers[binding] = internal_buffer.GetHandle();
    vertex_buffer_binding_state.offsets[binding] = offset;
    m_DirtyVertexBufferMask |= 1u << binding;
}

void CommandList_Vulkan::SetScissor(const Scissor &scissor)
{
	m_Scissor.offset = { scissor.offset.x, scissor.offset.y };
	m_Scissor.extent = { scissor.extent.width,scissor.extent.height };

	vkCmdSetScissor(m_CmdBuffer, 0, 1, &m_Scissor);
}

void CommandList_Vulkan::SetViewPort(const Viewport &viewport)
{
	m_Viewport.x = viewport.x;
	m_Viewport.y = viewport.y;
	m_Viewport.width = viewport.width;
	m_Viewport.height = viewport.height;
	m_Viewport.minDepth = viewport.minDepth;
	m_Viewport.maxDepth = viewport.maxDepth;

	vkCmdSetViewport(m_CmdBuffer, 0, 1, &m_Viewport);
}

void CommandList_Vulkan::FlushDescriptorSet(u32 set)
{
    QK_CORE_ASSERT((m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) != 0)

    auto& set_layout = m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set];
    auto& bindings = m_BindingState.descriptorBindings[set];

	uint32_t num_dynamic_offsets = 0;
	uint32_t dynamic_offsets[SET_BINDINGS_MAX_NUM];
	util::Hasher h;

    for (auto& b : set_layout.bindings) {
        switch (b.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].buffer.buffer);
                h.u64(bindings[b.binding + i].buffer.range);
#ifdef QK_DEBUG_BUILD
                if (bindings[b.binding + i].buffer.buffer == VK_NULL_HANDLE)
                    QK_CORE_LOGW_TAG("Graphic", "Buffer at Set: {}, Binding {} is not bounded. Performance waring!", set, b.binding);
#endif
                QK_CORE_ASSERT(num_dynamic_offsets < SET_BINDINGS_MAX_NUM)
                h.u32(bindings[b.binding + i].dynamicOffset);
                dynamic_offsets[num_dynamic_offsets++] = bindings[b.binding + i].dynamicOffset;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].buffer.buffer);
                h.u64(bindings[b.binding + i].buffer.range);
                QK_CORE_ASSERT(bindings[b.binding + i].buffer.buffer != VK_NULL_HANDLE)
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].image.imageView);
                h.pointer(bindings[b.binding + i].image.sampler);
                h.u32(bindings[b.binding + i].image.imageLayout);

#ifdef QK_DEBUG_BUILD
                if (bindings[b.binding + i].image.imageView == VK_NULL_HANDLE) {
                    QK_CORE_LOGE_TAG("Graphic", "Texture at Set: {}, Binding: {} is not bound.", set, b.binding);
                    QK_CORE_ASSERT(0)
                }
                if (bindings[b.binding + i].image.sampler == VK_NULL_HANDLE)
                {
                    QK_CORE_LOGE_TAG("Graphic", "Sampler at Set: {}, Binding: {} is not bound.", set, b.binding);
                    QK_CORE_ASSERT(0)
                }
#endif
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: // separate image
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].image.imageView);
                h.u32(bindings[b.binding + i].image.imageLayout);
                QK_CORE_ASSERT(bindings[b.binding + i].image.imageView != VK_NULL_HANDLE)
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER: // separate sampler
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].image.sampler);
                QK_CORE_ASSERT(bindings[b.binding + i].image.sampler != VK_NULL_HANDLE)
            }
            break;
        }
        default:
            QK_CORE_VERIFY(0, "Descriptor type not handled!")
            break;
        }
    }
    util::Hash hash = h.get();
    auto allocated = m_CurrentPipeline->GetLayout()->setAllocators[set]->Find(hash);

    // The descriptor set was not successfully cached, rebuild
    if (!allocated.second) {
        auto updata_template = m_CurrentPipeline->GetLayout()->updateTemplate[set];
        QK_CORE_ASSERT(updata_template)
        vkUpdateDescriptorSetWithTemplate(m_GraphicDevice->vkDevice, allocated.first, updata_template, bindings);
    }

    vkCmdBindDescriptorSets(m_CmdBuffer, (m_CurrentPipeline->GetBindingPoint() == PipeLineBindingPoint::GRAPHIC ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        m_CurrentPipeline->GetLayout()->handle, set, 1, &allocated.first, num_dynamic_offsets, dynamic_offsets);

    m_CurrentSets[set] = allocated.first;
}  

void CommandList_Vulkan::RebindDescriptorSet(u32 set)
{
    QK_CORE_ASSERT((m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) != 0)
    QK_CORE_ASSERT(m_CurrentSets[set] != nullptr)

    auto& set_layout = m_CurrentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set];
    auto& bindings = m_BindingState.descriptorBindings[set];

	u32 num_dynamic_offsets = 0;
	uint32_t dynamic_offsets[SET_BINDINGS_MAX_NUM];

    util::for_each_bit(set_layout.uniform_buffer_mask, [&](u32 binding) 
    {
        for (size_t i = 0; i < set_layout.vk_bindings[binding].descriptorCount; ++i) 
        {
            QK_CORE_ASSERT(num_dynamic_offsets < SET_BINDINGS_MAX_NUM)
            dynamic_offsets[num_dynamic_offsets++] = bindings[binding + i].dynamicOffset;
        }
    });

    vkCmdBindDescriptorSets(m_CmdBuffer, (m_CurrentPipeline->GetBindingPoint() == PipeLineBindingPoint::GRAPHIC ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        m_CurrentPipeline->GetLayout()->handle, set, 1, &m_CurrentSets[set], num_dynamic_offsets, dynamic_offsets);
}

void CommandList_Vulkan::DrawIndexed(u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset, u32 first_instance)
{
    QK_CORE_ASSERT(m_BindingState.indexBufferBindingState.buffer != VK_NULL_HANDLE)

    // Flush render state : update descriptor sets and bind vertex buffers 
    FlushRenderState();
    
    vkCmdDrawIndexed(m_CmdBuffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandList_Vulkan::Draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    // Flush render state : update descriptor sets and bind vertex buffers 
    FlushRenderState();

    vkCmdDraw(m_CmdBuffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandList_Vulkan::FlushRenderState()
{
    auto* pipeline_layout = m_CurrentPipeline->GetLayout();

    // 1. Flush dirty descriptor set
    u32 sets_need_update = pipeline_layout->combinedLayout.descriptorSetLayoutMask & m_DirtySetMask;
    util::for_each_bit(sets_need_update, [&](u32 set) { FlushDescriptorSet(set); });
    m_DirtySetMask &= ~sets_need_update;

    // If we update a set, we also bind dynamically
    m_DirtySetDynamicMask&= ~sets_need_update;

    // if only rebound dynamic uniform buffers with different offset,
    // we only need to rebinding descriptor set with different dynamic offsets
    u32 dynamic_sets_need_update = pipeline_layout->combinedLayout.descriptorSetLayoutMask & m_DirtySetDynamicMask;
    util::for_each_bit(dynamic_sets_need_update, [&](u32 set) {RebindDescriptorSet(set);});
    m_DirtySetDynamicMask&= ~dynamic_sets_need_update;

    // 2.Flush dirty vertex buffer
    auto& vertex_buffer_bindings = m_BindingState.vertexBufferBindingState;
    util::for_each_bit_range(m_DirtyVertexBufferMask, [&](u32 first_binding, u32 count) {
#ifdef QK_DEBUG_BUILD
        for (size_t binding = first_binding; binding < count; ++binding)
            QK_CORE_ASSERT(vertex_buffer_bindings.buffers[binding] != VK_NULL_HANDLE)
#endif
        vkCmdBindVertexBuffers(m_CmdBuffer, first_binding, count, vertex_buffer_bindings.buffers + first_binding, vertex_buffer_bindings.offsets + first_binding);
    });
    m_DirtyVertexBufferMask = 0;

}

}