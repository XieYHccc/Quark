#include "pch.h"
#include "Graphic/Vulkan/CommandList_Vulkan.h"
#include "Graphic/Vulkan/Device_Vulkan.h"
#include "Graphic/Vulkan/PipeLine_Vulkan.h"

namespace graphic {
CommandList_Vulkan::CommandList_Vulkan(Device_Vulkan* device, QueueType type)
    : CommandList(type), device_(device)
{
    CORE_DEBUG_ASSERT(device_ != nullptr)
    auto& vulkan_context = device_->context;
    VkDevice vk_device = device_->vkDevice;

    // Default values
    dirtyBits_ = 0;
    dirtySetBits_ = 0;
    currentPipeLine_ = nullptr;
    viewport_ = {};
    scissor_ = {};

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
        vk_image_barrier.image = internal_image.handle_;
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

void CommandList_Vulkan::BeginRenderPass(const RenderPassInfo &info)
{
    CORE_DEBUG_ASSERT(info.numColorAttachments < RenderPassInfo::MAX_COLOR_ATTHACHEMNT_NUM)
    CORE_DEBUG_ASSERT(info.numResolveAttachments < RenderPassInfo::MAX_COLOR_ATTHACHEMNT_NUM)

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
        color_attachments[i].imageView = internal_image.view_;
        color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].loadOp = convertLoadOp(info.colorAttatchemtsLoadOp[i]);
        color_attachments[i].storeOp = convertStoreOp(info.colorAttatchemtsStoreOp[i]);
        color_attachments[i].clearValue.color.float32[0] = info.clearColors[i].color[0];
        color_attachments[i].clearValue.color.float32[1] = info.clearColors[i].color[1];
        color_attachments[i].clearValue.color.float32[2] = info.clearColors[i].color[2];
        color_attachments[i].clearValue.color.float32[3] = info.clearColors[i].color[3];

        // internal swapchain image state tracking
        if (internal_image.isSwapChainImage_) {
            waitForSwapchainImage_ = true;
            swapChainWaitStages_ |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
    }

    // Resolve attatchments
    for (size_t i = 0; i < info.numResolveAttachments; ++i) {
        color_attachments[i].resolveImageView = ToInternal(info.resolveAttatchments[i]).view_;
        color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    // Depth attatchment
    if (info.depthAttatchment != nullptr) {
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = ToInternal(info.depthAttatchment).view_;
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = convertLoadOp(info.depthAttachmentLoadOp);
        depth_attachment.storeOp = convertStoreOp(info.depthAttachmentStoreOp);
        depth_attachment.clearValue.depthStencil.depth = info.ClearDepthStencil.depth_stencil.depth;

    }

    rendering_info.pColorAttachments = info.numColorAttachments > 0? color_attachments : nullptr;
    rendering_info.pDepthAttachment = info.depthAttatchment ? &depth_attachment : nullptr;
    //TODO: Support stencil test
    rendering_info.pStencilAttachment = nullptr;
    rendering_info.pNext = nullptr;

    device_->context->extendFunction.pVkCmdBeginRenderingKHR(cmdBuffer_, &rendering_info);   
}

void CommandList_Vulkan::EndRenderPass()
{
    device_->context->extendFunction.pVkCmdEndRenderingKHR(cmdBuffer_);
}

void CommandList_Vulkan::BindPushConstant(const void *data, size_t offset, size_t size)
{
    CORE_DEBUG_ASSERT(offset + size < Shader::PUSH_CONSTANT_DATA_SIZE)

    if (currentPipeLine_ != nullptr) {
        CORE_DEBUG_ASSERT(currentPipeLine_->layout_->pushConstant.size > 0)
        auto& push_constant = currentPipeLine_->layout_->pushConstant;
        vkCmdPushConstants(cmdBuffer_,
            currentPipeLine_->layout_->pipelineLayout,
            push_constant.stageFlags,
            push_constant.offset,
            push_constant.size,
            data);
    }
    
}

void CommandList_Vulkan::BindUniformBuffer(u32 set, u32 binding, const Buffer &buffer, uint64_t offset, uint64_t size)
{
    CORE_DEBUG_ASSERT(set < Shader::SHADER_RESOURCE_SET_MAX_NUM)
    CORE_DEBUG_ASSERT(binding < Shader::SET_BINDINGS_MAX_NUM)
    CORE_DEBUG_ASSERT(buffer.GetDesc().type == BufferType::UNIFORM_BUFFER)
    auto& internal_buffer = ToInternal(&buffer);
    auto& b = bindings_[set][binding];

    if (b.buffer.buffer == internal_buffer.handle_ && b.buffer.range == size) {
        if (b.buffer.offset) {
        
        }
    }

}

void CommandList_Vulkan::BindPipeLine(PipeLineType type, const PipeLine &pipeline)
{
    auto& internal_pipeline = ToInternal(&pipeline);
    CORE_DEBUG_ASSERT(internal_pipeline.pipeline_ != VK_NULL_HANDLE)

    if (type == PipeLineType::GRAPHIC)
        vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, internal_pipeline.pipeline_);
    else
        vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, internal_pipeline.pipeline_);

    currentPipeLine_ = &internal_pipeline;
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
}