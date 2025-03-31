#include "Quark/qkpch.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/Util/BitOperations.h"
#include "Quark/RHI/Vulkan/CommandList_Vulkan.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/RHI/Vulkan/PipeLine_Vulkan.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"

namespace quark::rhi {

constexpr VkImageAspectFlags _ConvertImageAspect(ImageAspect value)
{
    switch (value)
    {
    default:
    case rhi::ImageAspect::COLOR:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    case rhi::ImageAspect::DEPTH:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    case rhi::ImageAspect::STENCIL:
        return VK_IMAGE_ASPECT_STENCIL_BIT;
    case rhi::ImageAspect::LUMINANCE:
        return VK_IMAGE_ASPECT_PLANE_0_BIT;
    case rhi::ImageAspect::CHROMINANCE:
        return VK_IMAGE_ASPECT_PLANE_1_BIT;
    }
}

CommandList_Vulkan::CommandList_Vulkan(Device_Vulkan* device, QueueType type)
    : CommandList(type), m_device(device)
{
    QK_CORE_ASSERT(m_device != nullptr)
    auto& vulkan_context = m_device->vkContext;
    VkDevice vk_device = m_device->vkDevice;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // TODO: Support other queue types
    switch (m_queueType) {
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
    VK_CHECK(vkCreateCommandPool(vk_device, &poolInfo, nullptr, &m_cmdPool))

    // Allocate command buffer
    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandBufferCount = 1;
    commandBufferInfo.commandPool = m_cmdPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK(vkAllocateCommandBuffers(vk_device, &commandBufferInfo, &m_cmdBuffer))

    VkSemaphoreCreateInfo semCreateInfo = {};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(vk_device, &semCreateInfo, nullptr, &m_cmdCompleteSemaphore))
}

void CommandList_Vulkan::ResetAndBeginCmdBuffer()
{
    // Reset status
    vkResetCommandPool(m_device->vkDevice, m_cmdPool, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional
    VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo))

    m_waitForSwapchainImage = false;
    m_swapChainWaitStages = 0;
    m_imageBarriers.clear();
    m_bufferBarriers.clear();
    m_memoryBarriers.clear();
    state = CommandListState::IN_RECORDING;

    m_currentPipeline = nullptr;
    ResetBindingState();
}

CommandList_Vulkan::~CommandList_Vulkan()
{
    vkDestroySemaphore(m_device->vkDevice, m_cmdCompleteSemaphore, nullptr);
    vkDestroyCommandPool(m_device->vkDevice, m_cmdPool, nullptr); 
}

void CommandList_Vulkan::PipeLineBarriers(const PipelineMemoryBarrier *memoryBarriers, uint32_t memoryBarriersCount, const PipelineImageBarrier *imageBarriers, uint32_t iamgeBarriersCount, const PipelineBufferBarrier *bufferBarriers, uint32_t bufferBarriersCount)
{
    QK_CORE_ASSERT(bufferBarriers == nullptr);   // do not support buffer barrier for now

    QK_CORE_ASSERT(m_memoryBarriers.empty() && m_imageBarriers.empty() && m_bufferBarriers.empty())

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
        m_memoryBarriers.push_back(vk_memory_barrier);
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

        m_imageBarriers.push_back(vk_image_barrier);
    }
    
    // TODO:Support buffer barrier

    if (!m_memoryBarriers.empty() || !m_imageBarriers.empty() || !m_bufferBarriers.empty()) {
        VkDependencyInfo dependency_info = {};
        dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.memoryBarrierCount = static_cast<uint32_t>(m_memoryBarriers.size());
        dependency_info.pMemoryBarriers = m_memoryBarriers.data();
        dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(m_bufferBarriers.size());
        dependency_info.pBufferMemoryBarriers = m_bufferBarriers.data();
        dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(m_imageBarriers.size());
        dependency_info.pImageMemoryBarriers = m_imageBarriers.data();

        m_device->vkContext->extendFunction.pVkCmdPipelineBarrier2KHR(m_cmdBuffer, &dependency_info);

        m_memoryBarriers.clear();
        m_imageBarriers.clear();
        m_bufferBarriers.clear();
    }
}

void CommandList_Vulkan::ResetBindingState()
{
    m_bindingState = {};

    for (int i = 0; i < DESCRIPTOR_SET_MAX_NUM; i++)
        m_currentSets[i] = VK_NULL_HANDLE;

    m_dirtySetMask = 0;
    m_dirtyVertexBufferMask = 0;
    m_dirtySetRebindMask = 0;
}

void CommandList_Vulkan::BeginRenderPass(const RenderPassInfo2& renderPassInfo, const FrameBufferInfo& frameBufferInfo)
{
    QK_CORE_ASSERT(renderPassInfo.numColorAttachments < MAX_COLOR_ATTHACHEMNT_NUM)
        QK_CORE_ASSERT(frameBufferInfo.numResolveAttachments < renderPassInfo.numColorAttachments)

#if QK_DEBUG_BUILD
        if (state != CommandListState::IN_RECORDING)
            QK_CORE_LOGE_TAG("RHI", "You must call BeginRenderPass() in recording state.");
#endif

    // Change state
    state = CommandListState::IN_RENDERPASS;
    m_currentRenderPassInfo = renderPassInfo;
    m_currentPipeline = nullptr;
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
            m_waitForSwapchainImage = true;
            m_swapChainWaitStages |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
    }

    // Resolve attatchments
    for (size_t i = 0; i < frameBufferInfo.numResolveAttachments; ++i) {
        color_attachments[i].resolveImageView = ToInternal(frameBufferInfo.resolveAttatchments[i]).GetView();
        color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    // Depth attatchment
    if (renderPassInfo.depthAttachmentFormat != DataFormat::UNDEFINED) {
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = ToInternal(frameBufferInfo.depthAttachment).GetView();
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = convertLoadOp(frameBufferInfo.depthAttachmentLoadOp);
        depth_attachment.storeOp = convertStoreOp(frameBufferInfo.depthAttachmentStoreOp);
        depth_attachment.clearValue.depthStencil.depth = frameBufferInfo.clearDepthStencil.depth_stencil.depth;

    }

    rendering_info.pColorAttachments = renderPassInfo.numColorAttachments > 0 ? color_attachments : nullptr;
    rendering_info.pDepthAttachment = renderPassInfo.depthAttachmentFormat != DataFormat::UNDEFINED ? &depth_attachment : nullptr;
    //TODO: Support stencil test
    rendering_info.pStencilAttachment = nullptr;
    rendering_info.pNext = nullptr;

    m_device->vkContext->extendFunction.pVkCmdBeginRenderingKHR(m_cmdBuffer, &rendering_info);
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
//    m_currentPipeline = nullptr;
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
//    m_device->vkContext->extendFunction.pVkCmdBeginRenderingKHR(m_cmdBuffer, &rendering_info);   
//}

void CommandList_Vulkan::EndRenderPass()
{   
#if QK_DEBUG_BUILD
    if (state != CommandListState::IN_RENDERPASS) {
        QK_CORE_LOGE_TAG("RHI", "You must call BeginRenderPass() before calling EndRenderPass()");
    }
#endif
    // Set state back to in recording
    state = CommandListState::IN_RECORDING;
    m_device->vkContext->extendFunction.pVkCmdEndRenderingKHR(m_cmdBuffer);
}

const RenderPassInfo2& CommandList_Vulkan::GetCurrentRenderPassInfo() const
{
    return m_currentRenderPassInfo;
}

const PipeLine* CommandList_Vulkan::GetCurrentGraphicsPipeline() const
{
    return m_currentPipeline;
}


void CommandList_Vulkan::PushConstant(const void *data, uint32_t offset, uint32_t size)
{
    QK_CORE_ASSERT(offset + size < PUSH_CONSTANT_DATA_SIZE)

// #ifdef QK_DEBUG_BUILD
//     if (m_currentPipeline == nullptr) 
//     {
//         
// , "You can not bind a push constant before binding a pipeline.");
//         return;
//     }
//     if (m_currentPipeline->GetLayout()->combinedLayout.pushConstant.size == 0) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "Current pipeline's layout do not have a push constant");
//         return;
//     }
// #endif

    // auto& layout = *m_currentPipeline->GetLayout();
    // vkCmdPushConstants(m_cmdBuffer,
    //     layout.handle,
    //     layout.combinedLayout.pushConstant.stageFlags,
    //     offset,
    //     size,
    //     data);

    memcpy(m_bindingState.pushConstantData + offset, data, size);
	_SetDirtyFlags(COMMAND_LIST_DIRTY_PUSH_CONSTANTS_BIT);
}

void CommandList_Vulkan::BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer &buffer, uint64_t offset, uint64_t size)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

// #ifdef QK_DEBUG_BUILD
//     if (m_currentPipeline == nullptr) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "You must bind a pipeline before binding a uniform buffer.");
//         return;
//     }
//     if ((buffer.GetDesc().usageBits & BUFFER_USAGE_UNIFORM_BUFFER_BIT) == 0) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "CommandList_Vulkan::BindUniformBuffer : The bounded buffer doesn't has BUFFER_USAGE_UNIFORM_BUFFER_BIT");
//         return;
//     }
//     if ((m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) == 0 ||
//         (m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set].uniform_buffer_mask & (1u << binding))== 0) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "CommandList_Vulkan::BindUniformBuffer : Set: {}, binding: {} is not a uniforom buffer.", set, binding);
//         return;
//     }
// #endif

    auto& internal_buffer = ToInternal(&buffer);
    auto& b = m_bindingState.descriptorBindings[set][binding];

    if (b.buffer.buffer == internal_buffer.GetHandle() && b.buffer.range == size)
    {
        // if (b.dynamicOffset != offset) 
        // {
        //     m_dirtySetRebindMask|= 1u << set;
        //     b.dynamicOffset = (uint32_t)offset;
        // }

        m_dirtySetRebindMask |= 1u << set;
        b.dynamicOffset = (uint32_t)offset;
    }
    else 
    {
        b.buffer = {internal_buffer.GetHandle(), 0, size};
        b.dynamicOffset = (uint32_t)offset;
        m_dirtySetMask |= 1u << set;
    }

}

void CommandList_Vulkan::BindStorageBuffer(uint32_t set, uint32_t binding, const Buffer &buffer, u64 offset, u64 size)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

// #ifdef QK_DEBUG_BUILD
//     if (m_currentPipeline == nullptr)
//     {
//         QK_CORE_LOGE_TAG("RHI", "You must bind a pipeline before binding a storage buffer.");
//         return;
//     }
//     if ((buffer.GetDesc().usageBits & BUFFER_USAGE_STORAGE_BUFFER_BIT) == 0)
//     {
//         QK_CORE_LOGE_TAG("RHI", "CommandList_Vulkan::BindStorageBuffer : The bounded buffer doesn't has BUFFER_USAGE_STORAGE_BUFFER_BIT");
//         return;
//     }

//     if ((m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) == 0 ||
//         (m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set].storage_buffer_mask & (1u << binding)) == 0) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "CommandList_Vulkan::BindStorageBuffer : Set: {}, binding: {} is not a storage buffer.", set, binding);
//         return;
//     }
// #endif

    auto& internal_buffer = ToInternal(&buffer);
    auto& b = m_bindingState.descriptorBindings[set][binding];

    // if (b.buffer.buffer == internal_buffer.GetHandle() && b.buffer.range == size) 
    //     return;

    b.buffer = { internal_buffer.GetHandle(), offset, size };
    b.dynamicOffset = 0;
    m_dirtySetMask |= 1u << set;
}

void CommandList_Vulkan::BindImage(uint32_t set, uint32_t binding, const Image &image, ImageLayout layout)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

// #ifdef QK_DEBUG_BUILD
//     if (m_currentPipeline == nullptr) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "You must bind a pipeline before binding a image.");
//         return;
//     }
//     if (!(image.GetDesc().usageBits & IMAGE_USAGE_SAMPLING_BIT) &&
//         !(image.GetDesc().usageBits & IMAGE_USAGE_STORAGE_BIT)) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "Binded Image must with usage bits: IMAGE_USAGE_SAMPLING_BIT and IMAGE_USAGE_STORAGE_BIT");
//         return;
//     }
//     if (layout != ImageLayout::SHADER_READ_ONLY_OPTIMAL && layout != ImageLayout::GENERAL) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "Bind image's layout can only be SHADER_READ_ONLY_OPTIMAL and GENERAL");
//         return;
//     }
// #endif

    auto& internal_image = ToInternal(&image);
    auto& b = m_bindingState.descriptorBindings[set][binding];

    if (b.image.imageView == internal_image.GetView() && b.image.imageLayout == ConvertImageLayout(layout))
    {
        m_dirtySetRebindMask |= 1u << set;
        return;
    }

    b.image.imageView = internal_image.GetView();
    b.image.imageLayout = ConvertImageLayout(layout);
    m_dirtySetMask |= 1u << set;
    
}

void CommandList_Vulkan::BindPipeLine(const PipeLine &pipeline)
{
    auto& internal_pipeline = ToInternal(&pipeline);
    QK_CORE_ASSERT(internal_pipeline.GetHandle() != VK_NULL_HANDLE)

// #ifdef QK_DEBUG_BUILD
//     if (!m_currentRenderPassInfo.IsValid()) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "BindPipeLine()::You must call BeginRenderPass() before binding a pipeline.");
//         return;
//     }

//     const auto& render_pass_info = internal_pipeline.GetGraphicPipelineDesc().renderPassInfo;
//     if (render_pass_info.numColorAttachments != m_currentRenderPassInfo.numColorAttachments) 
//     {
//         QK_CORE_LOGE_TAG("RHI", "BindPipeLine()::The pipeline's color attachment number is not equal to the current render pass.");
//         return;
//     }

//     if (render_pass_info.depthAttachmentFormat != m_currentRenderPassInfo.depthAttachmentFormat)
//     {
//         QK_CORE_LOGE_TAG("RHI", "BindPipeLine()::The pipeline's depth attachment is not equal to the current render pass.");
//         return;
//     }
// #endif

    if (m_currentPipeline == &internal_pipeline)
        return;
    
    if (internal_pipeline.GetBindingPoint() == PipeLineBindingPoint::GRAPHIC)
        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal_pipeline.GetHandle());
    else
        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, internal_pipeline.GetHandle());

    // reset binding state if layout changed
    // if (!m_currentPipeline || m_currentPipeline->GetLayout() != internal_pipeline.GetLayout())
    // {
    //     m_dirtySetMask = 0;
    //     m_dirtySetRebindMask = 0;

    //     memset(m_bindingState.descriptorBindings, 0, sizeof(m_bindingState.descriptorBindings));

    //     for (int i = 0; i < DESCRIPTOR_SET_MAX_NUM; i++)
    //         m_currentSets[i] = VK_NULL_HANDLE;
    // }

    m_currentPipeline = &internal_pipeline;
}

void CommandList_Vulkan::BindSampler(uint32_t set, uint32_t binding, const Sampler& sampler)
{
    QK_CORE_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    QK_CORE_ASSERT(binding < SET_BINDINGS_MAX_NUM)

// #ifdef QK_DEBUG_BUILD
//     if (m_currentPipeline == nullptr)
//         QK_CORE_LOGE_TAG("RHI", "You must bind a pipeline before binding a sampler.");
// #endif

    auto& internal_sampler = ToInternal(&sampler);
    auto& b = m_bindingState.descriptorBindings[set][binding];

    if (b.image.sampler == internal_sampler.GetHandle())
    {
        m_dirtySetRebindMask |= 1u << set;
        return;
    }

    b.image.sampler = internal_sampler.GetHandle();
    m_dirtySetMask |= 1u << set;
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

	vkCmdCopyImageToBuffer(m_cmdBuffer, internal_image.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, internal_buffer.GetHandle(), 1, &copy_region);
}

void CommandList_Vulkan::BindIndexBuffer(const Buffer &buffer, u64 offset, const IndexBufferFormat format)
{
    auto& internal_buffer = ToInternal(&buffer);
    QK_CORE_ASSERT(internal_buffer.GetHandle() != VK_NULL_HANDLE)
    QK_CORE_ASSERT((buffer.GetDesc().usageBits & BUFFER_USAGE_INDEX_BUFFER_BIT) != 0)
    
    auto& index_buffer_binding_state = m_bindingState.indexBufferBindingState;
    // if (internal_buffer.GetHandle() == index_buffer_binding_state.buffer && offset == index_buffer_binding_state.offset && format == index_buffer_binding_state.format)
    //     return;

    index_buffer_binding_state.buffer = internal_buffer.GetHandle();
    index_buffer_binding_state.offset = offset;
    index_buffer_binding_state.format = format;

    vkCmdBindIndexBuffer(m_cmdBuffer, internal_buffer.GetHandle(), offset, (format == IndexBufferFormat::UINT16? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32));
}

void CommandList_Vulkan::BindVertexBuffer(uint32_t binding, const Buffer &buffer, u64 offset)
{
    auto& internal_buffer = ToInternal(&buffer);
    // TODO: add some state track for debuging here
    QK_CORE_ASSERT(binding < VERTEX_BUFFER_MAX_NUM)
    QK_CORE_ASSERT(buffer.GetDesc().usageBits & BUFFER_USAGE_VERTEX_BUFFER_BIT)

    auto& vertex_buffer_binding_state = m_bindingState.vertexBufferBindingState;

    // if (vertex_buffer_binding_state.buffers[binding] == internal_buffer.GetHandle() && vertex_buffer_binding_state.offsets[binding] == offset)
    //     return;

    vertex_buffer_binding_state.buffers[binding] = internal_buffer.GetHandle();
    vertex_buffer_binding_state.offsets[binding] = offset;
    m_dirtyVertexBufferMask |= 1u << binding;
}

void CommandList_Vulkan::SetScissor(const Scissor &scissor)
{
	m_scissor.offset = { scissor.offset.x, scissor.offset.y };
	m_scissor.extent = { scissor.extent.width,scissor.extent.height };

	vkCmdSetScissor(m_cmdBuffer, 0, 1, &m_scissor);
}

void CommandList_Vulkan::SetViewPort(const Viewport &viewport)
{
	m_viewport.x = viewport.x;
	m_viewport.y = viewport.y;
	m_viewport.width = viewport.width;
	m_viewport.height = viewport.height;
	m_viewport.minDepth = viewport.minDepth;
	m_viewport.maxDepth = viewport.maxDepth;

	vkCmdSetViewport(m_cmdBuffer, 0, 1, &m_viewport);
}

void CommandList_Vulkan::FlushDescriptorSet(uint32_t set)
{
    QK_CORE_ASSERT((m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) != 0)

    auto& set_layout = m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set];
    auto& bindings = m_bindingState.descriptorBindings[set];

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
                    QK_CORE_LOGW_TAG("RHI", "Buffer at Set: {}, Binding {} is not bounded. Performance waring!", set, b.binding);
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
                    QK_CORE_LOGE_TAG("RHI", "Texture at Set: {}, Binding: {} is not bound.", set, b.binding);
                    QK_CORE_ASSERT(0)
                }
                if (bindings[b.binding + i].image.sampler == VK_NULL_HANDLE)
                {
                    QK_CORE_LOGE_TAG("RHI", "Sampler at Set: {}, Binding: {} is not bound.", set, b.binding);
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
    auto allocated = m_currentPipeline->GetLayout()->setAllocators[set]->Find(hash);

    // The descriptor set was not successfully cached, rebuild
    if (!allocated.second) {
        auto updata_template = m_currentPipeline->GetLayout()->updateTemplate[set];
        QK_CORE_ASSERT(updata_template)
        vkUpdateDescriptorSetWithTemplate(m_device->vkDevice, allocated.first, updata_template, bindings);
    }

    vkCmdBindDescriptorSets(m_cmdBuffer, (m_currentPipeline->GetBindingPoint() == PipeLineBindingPoint::GRAPHIC ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        m_currentPipeline->GetLayout()->handle, set, 1, &allocated.first, num_dynamic_offsets, dynamic_offsets);

    m_currentSets[set] = allocated.first;
}  

void CommandList_Vulkan::RebindDescriptorSet(uint32_t set)
{
    QK_CORE_ASSERT((m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayoutMask & (1u << set)) != 0)
    QK_CORE_ASSERT(m_currentSets[set] != nullptr)

    auto& set_layout = m_currentPipeline->GetLayout()->combinedLayout.descriptorSetLayouts[set];
    auto& bindings = m_bindingState.descriptorBindings[set];

	uint32_t num_dynamic_offsets = 0;
	uint32_t dynamic_offsets[SET_BINDINGS_MAX_NUM];

    util::for_each_bit(set_layout.uniform_buffer_mask, [&](uint32_t binding) 
    {
        for (size_t i = 0; i < set_layout.vk_bindings[binding].descriptorCount; ++i) 
        {
            QK_CORE_ASSERT(num_dynamic_offsets < SET_BINDINGS_MAX_NUM)
            dynamic_offsets[num_dynamic_offsets++] = bindings[binding + i].dynamicOffset;
        }
    });

    vkCmdBindDescriptorSets(m_cmdBuffer, (m_currentPipeline->GetBindingPoint() == PipeLineBindingPoint::GRAPHIC ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        m_currentPipeline->GetLayout()->handle, set, 1, &m_currentSets[set], num_dynamic_offsets, dynamic_offsets);
}

void CommandList_Vulkan::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance)
{
    QK_CORE_ASSERT(m_bindingState.indexBufferBindingState.buffer != VK_NULL_HANDLE)

    // Flush render state : update descriptor sets and bind vertex buffers 
    FlushRenderState();
    
    vkCmdDrawIndexed(m_cmdBuffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandList_Vulkan::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    // Flush render state : update descriptor sets and bind vertex buffers 
    FlushRenderState();

    vkCmdDraw(m_cmdBuffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandList_Vulkan::FlushRenderState()
{
    QK_CORE_ASSERT(m_currentPipeline)
    
    const PipeLineLayout* pipeline_layout = m_currentPipeline->GetLayout();

    // 1. flush dirty descriptor set
    uint32_t sets_need_update = pipeline_layout->combinedLayout.descriptorSetLayoutMask & m_dirtySetMask;
    util::for_each_bit(sets_need_update, [&](uint32_t set) { FlushDescriptorSet(set); });
    m_dirtySetMask &= ~sets_need_update;

    // ff we update a set, we also bind dynamically
    m_dirtySetRebindMask&= ~sets_need_update;

    // if only rebound dynamic uniform buffers with different offset,
    // we only need to rebinding descriptor set with different dynamic offsets
    uint32_t dynamic_sets_need_update = pipeline_layout->combinedLayout.descriptorSetLayoutMask & m_dirtySetRebindMask;
    util::for_each_bit(dynamic_sets_need_update, [&](uint32_t set) {RebindDescriptorSet(set);});
    m_dirtySetRebindMask&= ~dynamic_sets_need_update;

    // 2. flush push constant
    if (_GetAndClearDirtyFlags(COMMAND_LIST_DIRTY_PUSH_CONSTANTS_BIT))
	{
		const VkPushConstantRange& range = pipeline_layout->combinedLayout.pushConstant;
		if (range.stageFlags != 0)
		{
			QK_CORE_ASSERT(range.offset == 0);
			vkCmdPushConstants(m_cmdBuffer, pipeline_layout->handle, range.stageFlags, 0, range.size, m_bindingState.pushConstantData);
		}
	}

    // 3. flush dirty vertex buffer
    auto& vertex_buffer_bindings = m_bindingState.vertexBufferBindingState;
    util::for_each_bit_range(m_dirtyVertexBufferMask, [&](uint32_t first_binding, uint32_t count) {
#ifdef QK_DEBUG_BUILD
        for (size_t binding = first_binding; binding < count; ++binding)
            QK_CORE_ASSERT(vertex_buffer_bindings.buffers[binding] != VK_NULL_HANDLE)
#endif
        vkCmdBindVertexBuffers(m_cmdBuffer, first_binding, count, vertex_buffer_bindings.buffers + first_binding, vertex_buffer_bindings.offsets + first_binding);
    });
    
    m_dirtyVertexBufferMask = 0;

}

CommandListDirtyFlagBits CommandList_Vulkan::_GetAndClearDirtyFlags(CommandListDirtyFlagBits flags)
{
    CommandListDirtyFlagBits ret = static_cast<CommandListDirtyFlagBits>(m_dirtyMask & flags);
    m_dirtyMask &= ~flags;
    return ret;
}

}