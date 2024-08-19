#include "Quark/qkpch.h"
#include "Quark/Graphic/Vulkan/CommandList_Vulkan.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#include "Quark/Graphic/Vulkan/PipeLine_Vulkan.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/Util/BitOperations.h"

namespace quark::graphic {
CommandList_Vulkan::CommandList_Vulkan(Device_Vulkan* device, QueueType type)
    : CommandList(type), device_(device)
{
    CORE_DEBUG_ASSERT(device_ != nullptr)
    auto& vulkan_context = device_->context;
    VkDevice vk_device = device_->vkDevice;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // TODO: Support other queue types
    switch (type_) {
    case QUEUE_TYPE_GRAPHICS:
        poolInfo.queueFamilyIndex = vulkan_context->graphicQueueIndex;
        break;
    case QUEUE_TYPE_ASYNC_COMPUTE:
        poolInfo.queueFamilyIndex = vulkan_context->computeQueueIndex;
        CORE_ASSERT(0)
        break;
    case QUEUE_TYPE_ASYNC_TRANSFER:
        poolInfo.queueFamilyIndex = vulkan_context->transferQueueIndex;
        CORE_ASSERT(0)
        break;
    default:
        CORE_ASSERT_MSG(0, "Queue Type not handled."); // queue type not handled
        break;
    }
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VK_CHECK(vkCreateCommandPool(vk_device, &poolInfo, nullptr, &cmdPool_))

    // Allocate command buffer
    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandBufferCount = 1;
    commandBufferInfo.commandPool = cmdPool_;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK(vkAllocateCommandBuffers(vk_device, &commandBufferInfo, &cmdBuffer_))

    VkSemaphoreCreateInfo semCreateInfo = {};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(vk_device, &semCreateInfo, nullptr, &cmdCompleteSemaphore_))
}

void CommandList_Vulkan::ResetAndBeginCmdBuffer()
{
    // Reset status
    vkResetCommandPool(device_->vkDevice, cmdPool_, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional
    VK_CHECK(vkBeginCommandBuffer(cmdBuffer_, &beginInfo))

    waitForSwapchainImage_ = false;
    swapChainWaitStages_ = 0;
    imageBarriers_.clear();
    bufferBarriers_.clear();
    memoryBarriers_.clear();
    state = CommandListState::IN_RECORDING;

    dirty_SetBits_ = 0;
    ditry_SetDynamicBits_ = 0;
    dirty_VertexBufferBits_ = 0;
    bindingState_ = {};
    currentPipeLine_ = nullptr;

}

CommandList_Vulkan::~CommandList_Vulkan()
{
    vkDestroySemaphore(device_->vkDevice, cmdCompleteSemaphore_, nullptr);
    vkDestroyCommandPool(device_->vkDevice, cmdPool_, nullptr); 
}

void CommandList_Vulkan::PipeLineBarriers(const PipelineMemoryBarrier *memoryBarriers, u32 memoryBarriersCount, const PipelineImageBarrier *imageBarriers, u32 iamgeBarriersCount, const PipelineBufferBarrier *bufferBarriers, u32 bufferBarriersCount)
{
    CORE_DEBUG_ASSERT(bufferBarriers == nullptr);   // do not support buffer barrier for now

    CORE_DEBUG_ASSERT(memoryBarriers_.empty() && imageBarriers_.empty() && bufferBarriers_.empty())

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
        memoryBarriers_.push_back(vk_memory_barrier);
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

        imageBarriers_.push_back(vk_image_barrier);
    }
    
    // TODO:Support buffer barrier

    if (!memoryBarriers_.empty() || !imageBarriers_.empty() || !bufferBarriers_.empty()) {
        VkDependencyInfo dependency_info = {};
        dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers_.size());
        dependency_info.pMemoryBarriers = memoryBarriers_.data();
        dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers_.size());
        dependency_info.pBufferMemoryBarriers = bufferBarriers_.data();
        dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers_.size());
        dependency_info.pImageMemoryBarriers = imageBarriers_.data();

        device_->context->extendFunction.pVkCmdPipelineBarrier2KHR(cmdBuffer_, &dependency_info);

        memoryBarriers_.clear();
        imageBarriers_.clear();
        bufferBarriers_.clear();
    }
}

void CommandList_Vulkan::ResetBindingStatus()
{
    bindingState_ = {};

    for (int i = 0; i < DESCRIPTOR_SET_MAX_NUM; i++) {
        currentSets[i] = VK_NULL_HANDLE;
    }

    dirty_SetBits_ = 0;
    dirty_VertexBufferBits_ = 0;
    ditry_SetDynamicBits_ = 0;
}

void CommandList_Vulkan::BeginRenderPass(const RenderPassInfo &info)
{
    CORE_DEBUG_ASSERT(info.numColorAttachments < RenderPassInfo::MAX_COLOR_ATTHACHEMNT_NUM)
    CORE_DEBUG_ASSERT(info.numResolveAttachments < RenderPassInfo::MAX_COLOR_ATTHACHEMNT_NUM)

#if QK_DEBUG_BUILD
    if (state != CommandListState::IN_RECORDING) {
        CORE_LOGE("You must call BeginRenderPass() in recording state.")
    }
#endif

    // Change state
    state = CommandListState::IN_RENDERPASS;
    currentRenderPassInfo_ = &info;
    currentPipeLine_ = nullptr;
    ResetBindingStatus();

    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.layerCount = 1;
    rendering_info.renderArea.offset.x = 0;
    rendering_info.renderArea.offset.y = 0;
    rendering_info.renderArea.extent.width = 0;
    rendering_info.renderArea.extent.height = 0;
    rendering_info.colorAttachmentCount = info.numColorAttachments;

    VkRenderingAttachmentInfo color_attachments[RenderPassInfo::MAX_COLOR_ATTHACHEMNT_NUM] = {};
    VkRenderingAttachmentInfo depth_attachment = {};

    auto convertLoadOp = [](RenderPassInfo::AttachmentLoadOp op) -> VkAttachmentLoadOp {
        switch (op) {
        default:
        case RenderPassInfo::AttachmentLoadOp::CLEAR:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case RenderPassInfo::AttachmentLoadOp::LOAD:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RenderPassInfo::AttachmentLoadOp::DONTCARE:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
    };

    auto convertStoreOp = [](RenderPassInfo::AttachmentStoreOp op) ->VkAttachmentStoreOp {
        switch (op) {
        default:
        case RenderPassInfo::AttachmentStoreOp::STORE:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case RenderPassInfo::AttachmentStoreOp::DONTCARE:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
    };

    // Color Attachments
    for (size_t i = 0; i < info.numColorAttachments; ++i) {
        const auto image = info.colorAttachments[i];
        const ImageDesc& image_desc = image->GetDesc();
        auto& internal_image = ToInternal(image);

        rendering_info.renderArea.extent.width = std::max(rendering_info.renderArea.extent.width, image_desc.width);
		rendering_info.renderArea.extent.height = std::max(rendering_info.renderArea.extent.height, image_desc.height);

        color_attachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachments[i].imageView = internal_image.GetView();
        color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].loadOp = convertLoadOp(info.colorAttatchemtsLoadOp[i]);
        color_attachments[i].storeOp = convertStoreOp(info.colorAttatchemtsStoreOp[i]);
        color_attachments[i].clearValue.color.float32[0] = info.clearColors[i].color[0];
        color_attachments[i].clearValue.color.float32[1] = info.clearColors[i].color[1];
        color_attachments[i].clearValue.color.float32[2] = info.clearColors[i].color[2];
        color_attachments[i].clearValue.color.float32[3] = info.clearColors[i].color[3];

        // internal swapchain image state tracking
        if (internal_image.IsSwapChainImage()) {
            waitForSwapchainImage_ = true;
            swapChainWaitStages_ |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
    }

    // Resolve attatchments
    for (size_t i = 0; i < info.numResolveAttachments; ++i) {
        color_attachments[i].resolveImageView = ToInternal(info.resolveAttatchments[i]).GetView();
        color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    // Depth attatchment
    if (info.depthAttachment != nullptr) {
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = ToInternal(info.depthAttachment).GetView();
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = convertLoadOp(info.depthAttachmentLoadOp);
        depth_attachment.storeOp = convertStoreOp(info.depthAttachmentStoreOp);
        depth_attachment.clearValue.depthStencil.depth = info.ClearDepthStencil.depth_stencil.depth;

    }

    rendering_info.pColorAttachments = info.numColorAttachments > 0? color_attachments : nullptr;
    rendering_info.pDepthAttachment = info.depthAttachment ? &depth_attachment : nullptr;
    //TODO: Support stencil test
    rendering_info.pStencilAttachment = nullptr;
    rendering_info.pNext = nullptr;

    device_->context->extendFunction.pVkCmdBeginRenderingKHR(cmdBuffer_, &rendering_info);   
}

void CommandList_Vulkan::EndRenderPass()
{   
#if QK_DEBUG_BUILD
    if (state != CommandListState::IN_RENDERPASS) {
        CORE_LOGE("You must call BeginRenderPass() before calling EndRenderPass()")
    }
#endif
    // Set state back to in recording
    state = CommandListState::IN_RECORDING;
    device_->context->extendFunction.pVkCmdEndRenderingKHR(cmdBuffer_);
}

void CommandList_Vulkan::BindPushConstant(const void *data, size_t offset, size_t size)
{
    CORE_DEBUG_ASSERT(offset + size < PUSH_CONSTANT_DATA_SIZE)

#ifdef QK_DEBUG_BUILD
    if (currentPipeLine_ == nullptr) {
        CORE_LOGE("You can not bind a push constant before binding a pipeline.")
    }
    if (currentPipeLine_->GetLayout()->pushConstant.size == 0) {
        CORE_LOGE("Current pipeline's layout do not have a push constant")
    }
#endif

    auto& layout = *currentPipeLine_->GetLayout();
    vkCmdPushConstants(cmdBuffer_,
        layout.handle,
        layout.pushConstant.stageFlags,
        offset,
        size,
        data);
    
}

void CommandList_Vulkan::BindUniformBuffer(u32 set, u32 binding, const Buffer &buffer, u64 offset, u64 size)
{
    CORE_DEBUG_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    CORE_DEBUG_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (currentPipeLine_ == nullptr) {
        CORE_LOGE("You must bind a pipeline before binding a uniform buffer.")
    }
    if ((buffer.GetDesc().usageBits & BUFFER_USAGE_UNIFORM_BUFFER_BIT) == 0) {
        CORE_LOGE("CommandList_Vulkan::BindUniformBuffer : The bounded buffer doesn't has BUFFER_USAGE_UNIFORM_BUFFER_BIT")
    }
    if ((currentPipeLine_->GetLayout()->setLayoutMask & (1u << set)) == 0 ||
        (currentPipeLine_->GetLayout()->setLayouts[set].uniform_buffer_mask & (1u << binding))== 0) {
        CORE_LOGE("CommandList_Vulkan::BindUniformBuffer : Set: {}, binding: {} is not a uniforom buffer.", set, binding)
    }
#endif

    auto& internal_buffer = ToInternal(&buffer);
    auto& b = bindingState_.descriptorBindings[set][binding];

    if (b.buffer.buffer == internal_buffer.GetHandle() && b.buffer.range == size) {
        if (b.dynamicOffset != offset) {
            ditry_SetDynamicBits_ |= 1u << set;
            b.dynamicOffset = offset;
        }
    }
    else {
        b.buffer = {internal_buffer.GetHandle(), 0, size};
        b.dynamicOffset = offset;
        dirty_SetBits_ |= 1u << set;
    }

}

void CommandList_Vulkan::BindStorageBuffer(u32 set, u32 binding, const Buffer &buffer, u64 offset, u64 size)
{
    CORE_DEBUG_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    CORE_DEBUG_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (currentPipeLine_ == nullptr) {
        CORE_LOGE("You must bind a pipeline before binding a storage buffer.")
    }
    if ((buffer.GetDesc().usageBits & BUFFER_USAGE_STORAGE_BUFFER_BIT) == 0) {
        CORE_LOGE("CommandList_Vulkan::BindStorageBuffer : The bounded buffer doesn't has BUFFER_USAGE_STORAGE_BUFFER_BIT")
    }
    if ((currentPipeLine_->GetLayout()->setLayoutMask & (1u << set)) == 0 ||
        (currentPipeLine_->GetLayout()->setLayouts[set].storage_buffer_mask & (1u << binding))== 0) {
        CORE_LOGE("CommandList_Vulkan::BindStorageBuffer : Set: {}, binding: {} is not a storage buffer.", set, binding)
    }
#endif

    auto& internal_buffer = ToInternal(&buffer);
    auto& b = bindingState_.descriptorBindings[set][binding];

    if (b.buffer.buffer == internal_buffer.GetHandle() && b.buffer.range == size) 
        return;

    b.buffer = {internal_buffer.GetHandle(), offset, size};
    b.dynamicOffset = 0;
    dirty_SetBits_ |= 1u << set;


}

void CommandList_Vulkan::BindImage(u32 set, u32 binding, const Image &image, ImageLayout layout)
{
    CORE_DEBUG_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    CORE_DEBUG_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (currentPipeLine_ == nullptr) {
        CORE_LOGE("You must bind a pipeline before binding a image.")
    }
    if (!(image.GetDesc().usageBits & IMAGE_USAGE_SAMPLING_BIT) &&
        !(image.GetDesc().usageBits & IMAGE_USAGE_STORAGE_BIT)) {
        CORE_LOGE("Binded Image must with usage bits: IMAGE_USAGE_SAMPLING_BIT and IMAGE_USAGE_STORAGE_BIT")
    }
    if (layout != ImageLayout::SHADER_READ_ONLY_OPTIMAL && layout != ImageLayout::GENERAL) {
        CORE_LOGE("Bind image's layout can only be SHADER_READ_ONLY_OPTIMAL and GENERAL")
    }
#endif

    auto& internal_image = ToInternal(&image);
    auto& b = bindingState_.descriptorBindings[set][binding];

    if (b.image.imageView == internal_image.GetView() && b.image.imageLayout == ConvertImageLayout(layout))
        return;

    b.image.imageView = internal_image.GetView();
    b.image.imageLayout = ConvertImageLayout(layout);
    dirty_SetBits_ |= 1u << set;
    
}

void CommandList_Vulkan::BindPipeLine(const PipeLine &pipeline)
{
    auto& internal_pipeline = ToInternal(&pipeline);
    CORE_DEBUG_ASSERT(internal_pipeline.GetHandle() != VK_NULL_HANDLE)

#ifdef QK_DEBUG_BUILD
    if (currentRenderPassInfo_ == nullptr) {
        CORE_LOGE("BindPipeLine()::You must call BeginRenderPass() before binding a pipeline.")
        return;
    }
    const auto& render_pass_info = internal_pipeline.GetRenderPassInfo();
    if (render_pass_info.numColorAttachments != currentRenderPassInfo_->numColorAttachments) {
        CORE_LOGE("BindPipeLine()::The pipeline's color attachment number is not equal to the current render pass.")
        return;
    }

    if (render_pass_info.depthAttachmentFormat != currentRenderPassInfo_->depthAttachmentFormat) {
        CORE_LOGE("BindPipeLine()::The pipeline's depth attachment is not equal to the current render pass.")
        return;
    }

    for (size_t i = 0; i < render_pass_info.numColorAttachments; i++) {
        if (render_pass_info.colorAttachmentFormats[i] != currentRenderPassInfo_->colorAttachments[i]->GetDesc().format) {
            CORE_LOGE("BindPipeLine()::The pipeline's color attachment format[{}] is not equal to the current render pass.", i)
            return;
        
        }
    }
#endif

    if (currentPipeLine_ == &internal_pipeline) {
        return;
    }
    
    if (internal_pipeline.GetType() == PipeLineType::GRAPHIC)
        vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, internal_pipeline.GetHandle());
    else
        vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, internal_pipeline.GetHandle());

    // Reset status
    currentPipeLine_ = &internal_pipeline;
    ResetBindingStatus();
}

void CommandList_Vulkan::BindSampler(u32 set, u32 binding, const Sampler& sampler)
{
    CORE_DEBUG_ASSERT(set < DESCRIPTOR_SET_MAX_NUM)
    CORE_DEBUG_ASSERT(binding < SET_BINDINGS_MAX_NUM)

#ifdef QK_DEBUG_BUILD
    if (currentPipeLine_ == nullptr) {
        CORE_LOGE("You must bind a pipeline before binding a sampler.")
    }
#endif

    auto& internal_sampler = ToInternal(&sampler);
    auto& b = bindingState_.descriptorBindings[set][binding];

    if (b.image.sampler == internal_sampler.GetHandle())
        return;

    b.image.sampler = internal_sampler.GetHandle();
    dirty_SetBits_ |= 1u << set;
}

void CommandList_Vulkan::BindIndexBuffer(const Buffer &buffer, u64 offset, const IndexBufferFormat format)
{
    auto& internal_buffer = ToInternal(&buffer);
    CORE_DEBUG_ASSERT(internal_buffer.GetHandle() != VK_NULL_HANDLE)
    CORE_DEBUG_ASSERT((buffer.GetDesc().usageBits & BUFFER_USAGE_INDEX_BUFFER_BIT) != 0)
    
    auto& index_buffer_binding_state = bindingState_.indexBufferBindingState;
    if (internal_buffer.GetHandle() == index_buffer_binding_state.buffer &&
        offset == index_buffer_binding_state.offset &&
        format == index_buffer_binding_state.format) {
        return;
    }

    index_buffer_binding_state.buffer = internal_buffer.GetHandle();
    index_buffer_binding_state.offset = offset;
    index_buffer_binding_state.format = format;

    vkCmdBindIndexBuffer(cmdBuffer_, internal_buffer.GetHandle(), offset, (format == IndexBufferFormat::UINT16? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32));
}

void CommandList_Vulkan::BindVertexBuffer(u32 binding, const Buffer &buffer, u64 offset)
{
    auto& internal_buffer = ToInternal(&buffer);
    // TODO: add some state track for debuging here
    CORE_DEBUG_ASSERT(binding < VERTEX_BUFFER_MAX_NUM)
    CORE_DEBUG_ASSERT(buffer.GetDesc().usageBits & BUFFER_USAGE_VERTEX_BUFFER_BIT)

    auto& vertex_buffer_binding_state = bindingState_.vertexBufferBindingState;
    if (vertex_buffer_binding_state.buffers[binding] == internal_buffer.GetHandle() &&
        vertex_buffer_binding_state.offsets[binding] == offset) {
        return;
    }

    vertex_buffer_binding_state.buffers[binding] = internal_buffer.GetHandle();
    vertex_buffer_binding_state.offsets[binding] = offset;
    dirty_VertexBufferBits_ |= 1u << binding;

}

void CommandList_Vulkan::SetScissor(const Scissor &scissor)
{
	scissor_.offset = { scissor.offset.x, scissor.offset.y };
	scissor_.extent = { scissor.extent.width,scissor.extent.height };

	vkCmdSetScissor(cmdBuffer_, 0, 1, &scissor_);
}

void CommandList_Vulkan::SetViewPort(const Viewport &viewport)
{
	viewport_.x = viewport.x;
	viewport_.y = viewport.y;
	viewport_.width = viewport.width;
	viewport_.height = viewport.height;
	viewport_.minDepth = viewport.minDepth;
	viewport_.maxDepth = viewport.maxDepth;

	vkCmdSetViewport(cmdBuffer_, 0, 1, &viewport_);
}

void CommandList_Vulkan::Flush_DescriptorSet(u32 set)
{
    CORE_DEBUG_ASSERT((currentPipeLine_->GetLayout()->setLayoutMask & (1u << set)) != 0)

    auto& set_layout = currentPipeLine_->GetLayout()->setLayouts[set];
    auto& bindings = bindingState_.descriptorBindings[set];

	u32 num_dynamic_offsets = 0;
	u32 dynamic_offsets[SET_BINDINGS_MAX_NUM];
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
                CORE_LOGW("Buffer at Set: {}, Binding {} is not bounded. Performance waring!", set, b.binding)
#endif
                CORE_DEBUG_ASSERT(num_dynamic_offsets < SET_BINDINGS_MAX_NUM)
                dynamic_offsets[num_dynamic_offsets++] = bindings[b.binding + i].dynamicOffset;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].buffer.buffer);
                h.u64(bindings[b.binding + i].buffer.range);
                CORE_DEBUG_ASSERT(bindings[b.binding + i].buffer.buffer != VK_NULL_HANDLE)
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
                    CORE_LOGC("Texture at Set: {}, Binding: {} is not bound.", set, b.binding)
                    CORE_DEBUG_ASSERT(0)
                }
                if (bindings[b.binding + i].image.sampler == VK_NULL_HANDLE)
                {
                    CORE_LOGC("Sampler at Set: {}, Binding: {} is not bound.", set, b.binding)
                    CORE_DEBUG_ASSERT(0)
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
                CORE_DEBUG_ASSERT(bindings[b.binding + i].image.imageView != VK_NULL_HANDLE)
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER: // separate sampler
        {
            for (size_t i = 0; i < b.descriptorCount; ++i) {
                h.pointer(bindings[b.binding + i].image.sampler);
                CORE_DEBUG_ASSERT(bindings[b.binding + i].image.sampler != VK_NULL_HANDLE)
            }
            break;
        }
        default:
            CORE_ASSERT_MSG(0, "Descriptor type not handled!")
            break;
        }
    }
    util::Hash hash = h.get();
    auto allocated = currentPipeLine_->GetLayout()->setAllocators[set]->find(hash);

    // The descriptor set was not successfully cached, rebuild
    if (!allocated.second) {
        auto updata_template = currentPipeLine_->GetLayout()->updateTemplate[set];
        CORE_DEBUG_ASSERT(updata_template)
        vkUpdateDescriptorSetWithTemplate(device_->vkDevice, allocated.first, updata_template, bindings);
    }

    vkCmdBindDescriptorSets(cmdBuffer_, (currentPipeLine_->GetType() == PipeLineType::GRAPHIC? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        currentPipeLine_->GetLayout()->handle, set, 1, &allocated.first, num_dynamic_offsets, dynamic_offsets);

    currentSets[set] = allocated.first;
}  

void CommandList_Vulkan::Rebind_DescriptorSet(u32 set)
{
    CORE_DEBUG_ASSERT((currentPipeLine_->GetLayout()->setLayoutMask & (1u << set)) != 0)
    CORE_DEBUG_ASSERT(currentSets[set] != nullptr)

    auto& set_layout = currentPipeLine_->GetLayout()->setLayouts[set];
    auto& bindings = bindingState_.descriptorBindings[set];

	u32 num_dynamic_offsets = 0;
	u32 dynamic_offsets[SET_BINDINGS_MAX_NUM];

    util::for_each_bit(set_layout.uniform_buffer_mask, [&](u32 binding) {
        for (size_t i = 0; i < set_layout.vk_bindings[binding].descriptorCount; ++i) {
            CORE_DEBUG_ASSERT(num_dynamic_offsets < SET_BINDINGS_MAX_NUM)
            dynamic_offsets[num_dynamic_offsets++] = bindings[binding + i].dynamicOffset;
        }
    });

    vkCmdBindDescriptorSets(cmdBuffer_, (currentPipeLine_->GetType() == PipeLineType::GRAPHIC? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        currentPipeLine_->GetLayout()->handle, set, 1, &currentSets[set], num_dynamic_offsets, dynamic_offsets);
}

void CommandList_Vulkan::DrawIndexed(u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset, u32 first_instance)
{
    CORE_DEBUG_ASSERT(bindingState_.indexBufferBindingState.buffer != VK_NULL_HANDLE)

    // Flush render state : update descriptor sets and bind vertex buffers 
    Flush_RenderState();
    
    vkCmdDrawIndexed(cmdBuffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandList_Vulkan::Draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    // Flush render state : update descriptor sets and bind vertex buffers 
    Flush_RenderState();

    vkCmdDraw(cmdBuffer_, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandList_Vulkan::Flush_RenderState()
{
    auto* pipeline_layout = currentPipeLine_->GetLayout();

    // 1. Flush dirty descriptor set
    u32 sets_need_update = pipeline_layout->setLayoutMask & dirty_SetBits_;
    util::for_each_bit(sets_need_update, [&](u32 set) { Flush_DescriptorSet(set);});
    dirty_SetBits_ &= ~sets_need_update;

    // If we update a set, we also bind dynamically
    ditry_SetDynamicBits_ &= ~sets_need_update;

    // if only rebound dynamic uniform buffers with different offset,
    // we only need to rebinding descriptor set with different dynamic offsets
    u32 dynamic_sets_need_update = pipeline_layout->setLayoutMask & ditry_SetDynamicBits_;
    util::for_each_bit(dynamic_sets_need_update, [&](u32 set) {Rebind_DescriptorSet(set);});
    ditry_SetDynamicBits_ &= ~dynamic_sets_need_update;

    // 2.Flush dirty vertex buffer
    auto& vertex_buffer_bindings = bindingState_.vertexBufferBindingState;
    util::for_each_bit_range(dirty_VertexBufferBits_, [&](u32 first_binding, u32 count) {
#ifdef QK_DEBUG_BUILD
        for (size_t binding = first_binding; binding < count; ++binding) {
            CORE_DEBUG_ASSERT(vertex_buffer_bindings.buffers[binding] != VK_NULL_HANDLE)
        }
#endif
        vkCmdBindVertexBuffers(cmdBuffer_, first_binding, count, vertex_buffer_bindings.buffers + first_binding, vertex_buffer_bindings.offsets + first_binding);
    });
    dirty_VertexBufferBits_ = 0;

}

}