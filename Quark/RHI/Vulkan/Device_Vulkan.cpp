#include "Quark/qkpch.h"
#define VMA_IMPLEMENTATION
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Math/Util.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Events/EventManager.h"
#include "Quark/RHI/Vulkan/Shader_Vulkan.h"

namespace quark::rhi {

void Device_Vulkan::CommandQueue::init(Device_Vulkan *device, QueueType type)
{
    this->device = device;
    this->type = type;

    switch (type) {
    case QUEUE_TYPE_GRAPHICS:
        queue = device->vkContext->graphicQueue;
        break;
    case QUEUE_TYPE_ASYNC_COMPUTE:
        queue = device->vkContext->computeQueue;
        break;
    case QUEUE_TYPE_ASYNC_TRANSFER:
        queue = device->vkContext->transferQueue;
        break;
    default:
        QK_CORE_ASSERT(0)
        break;
    }
}

void Device_Vulkan::CommandQueue::submit(VkFence fence)
{
    QK_CORE_ASSERT(!submissions.empty() || fence != VK_NULL_HANDLE)

    std::vector<VkSubmitInfo2> submit_infos(submissions.size());
    for (size_t i = 0; i < submissions.size(); ++i) 
    {
        auto& info = submit_infos[i];
        auto& submission = submissions[i];
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.commandBufferInfoCount = (uint32_t)submission.cmdInfos.size();
        info.pCommandBufferInfos = submission.cmdInfos.data();
        info.signalSemaphoreInfoCount = (uint32_t)submission.signalSemaphoreInfos.size();
        info.pSignalSemaphoreInfos = submission.signalSemaphoreInfos.data();
        info.waitSemaphoreInfoCount = (uint32_t)submission.waitSemaphoreInfos.size();
        info.pWaitSemaphoreInfos = submission.waitSemaphoreInfos.data();
    }

    device->vkContext->extendFunction.pVkQueueSubmit2KHR(queue, (uint32_t)submit_infos.size(), submit_infos.data(), fence);
    
    // Clear submissions
    for(auto& submission : submissions)
    {
        submission.cmdInfos.clear();
        submission.signalSemaphoreInfos.clear();
        submission.waitSemaphoreInfos.clear();
    }

    submissions.clear();
}

void Device_Vulkan::PerFrameData::init(Device_Vulkan *device)
{
    QK_CORE_ASSERT(device->vkDevice != VK_NULL_HANDLE)
    this->device = device;
    this->vmaAllocator = this->device->vmaAllocator;

    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) {
        // Create a fence per queue
        VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = 0;
        fence_create_info.pNext = nullptr;
        vkCreateFence(device->vkDevice, &fence_create_info, nullptr, &queueFences[i]);
    }
    
    // Create semaphore
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphore_create_info.pNext = nullptr;
    vkCreateSemaphore(device->vkDevice, &semaphore_create_info, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device->vkDevice, &semaphore_create_info, nullptr, &imageReleaseSemaphore);
}

void Device_Vulkan::PerFrameData::clear()
{
    VkDevice vk_device = device->vkDevice;

    // Destroy deferred destroyed resources
    for (auto& sampler : garbageSamplers) 
        vkDestroySampler(vk_device, sampler, nullptr);
    for (auto& view : grabageViews) 
        vkDestroyImageView(vk_device, view, nullptr);
    for (auto& buffer : garbageBuffers) 
        vmaDestroyBuffer(vmaAllocator, buffer.first, buffer.second);
    for (auto& image : garbageImages) 
        vmaDestroyImage(vmaAllocator, image.first, image.second);
    for (auto& pipeline : garbagePipelines) 
        vkDestroyPipeline(vk_device, pipeline, nullptr);
    for (auto& shaderModule_ : garbageShaderModules)
        vkDestroyShaderModule(vk_device, shaderModule_, nullptr);

    garbageSamplers.clear();
    garbageBuffers.clear();
    grabageViews.clear();
    garbageImages.clear();
    garbagePipelines.clear();
    garbageShaderModules.clear();
}

void Device_Vulkan::PerFrameData::reset()
{
    if (!waitedFences.empty()) 
    {
        vkResetFences(device->vkDevice, (uint32_t)waitedFences.size(), waitedFences.data());
        waitedFences.clear();
    }

    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++)
        cmdListCount[i] = 0;

    imageAvailableSemaphoreConsumed = false;

    // destroy deferred-destroyed resources
    clear();
}

void Device_Vulkan::PerFrameData::destroy()
{
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) {
        for (size_t j = 0; j < cmdLists[i].size(); j++) {
            delete cmdLists[i][j];
            cmdLists[i][j] = nullptr;
        }
        cmdLists[i].clear();

        vkDestroyFence(device->vkDevice, queueFences[i], nullptr);
    }

    vkDestroySemaphore(device->vkDevice, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device->vkDevice, imageReleaseSemaphore, nullptr);

    clear();
}


void Device_Vulkan::CopyCmdAllocator::init(Device_Vulkan *device)
{
    m_Device = device;
}

void Device_Vulkan::CopyCmdAllocator::destroy()
{   
    // Make sure all allocated cmd are in free list
    vkQueueWaitIdle(m_Device->m_queues[QUEUE_TYPE_ASYNC_TRANSFER].queue);
    for (auto& x : m_FreeList)
    {
        vkDestroyCommandPool(m_Device->vkDevice, x.transferCmdPool, nullptr);
        vkDestroyCommandPool(m_Device->vkDevice, x.transitionCmdPool, nullptr);
        vkDestroyFence(m_Device->vkDevice, x.fence, nullptr);
        vkDestroySemaphore(m_Device->vkDevice, x.semaphores[0], nullptr);
        vkDestroySemaphore(m_Device->vkDevice, x.semaphores[1], nullptr);
    }

    m_FreeList.clear();
}

Device_Vulkan::CopyCmdAllocator::CopyCmd Device_Vulkan::CopyCmdAllocator::allocate(VkDeviceSize required_buffer_size)
{
    CopyCmd cmd;

    // Try to find a suitable staging buffer in free list
    m_Locker.lock();
    for (size_t i = 0; i < m_FreeList.size(); ++i) 
    {
        if (m_FreeList[i].stageBuffer->GetDesc().size >= required_buffer_size) 
        {
            if (vkGetFenceStatus(m_Device->vkDevice, m_FreeList[i].fence) == VK_SUCCESS)
            {
                cmd = std::move(m_FreeList[i]);
                std::swap(m_FreeList[i], m_FreeList.back());
                m_FreeList.pop_back();
                break;
            }
        }
    }
    m_Locker.unlock();

    if (!cmd.isValid()) // No suitable staging buffer founded
    { 
        // Create command pool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_Device->vkContext->transferQueueIndex;
        VK_CHECK(vkCreateCommandPool(m_Device->vkDevice, &poolInfo, nullptr, &cmd.transferCmdPool))

        poolInfo.queueFamilyIndex = m_Device->vkContext->graphicQueueIndex;
        VK_CHECK(vkCreateCommandPool(m_Device->vkDevice, &poolInfo, nullptr, &cmd.transitionCmdPool))

        // Allocate command buffer
        VkCommandBufferAllocateInfo commandBufferInfo = {};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferInfo.commandBufferCount = 1;
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandPool = cmd.transferCmdPool;
        VK_CHECK(vkAllocateCommandBuffers(m_Device->vkDevice, &commandBufferInfo, &cmd.transferCmdBuffer))

        commandBufferInfo.commandPool = cmd.transitionCmdPool;
        VK_CHECK(vkAllocateCommandBuffers(m_Device->vkDevice, &commandBufferInfo, &cmd.transitionCmdBuffer))

        // Create fence
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VK_CHECK(vkCreateFence(m_Device->vkDevice, &fenceInfo, nullptr, &cmd.fence))

        // Create Semaphores
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK(vkCreateSemaphore(m_Device->vkDevice, &semaphoreInfo, nullptr, &cmd.semaphores[0]));
        VK_CHECK(vkCreateSemaphore(m_Device->vkDevice, &semaphoreInfo, nullptr, &cmd.semaphores[1]));

        // Create staging buffer
        BufferDesc bufferDesc;
        bufferDesc.domain = BufferMemoryDomain::CPU;
        bufferDesc.size = math::GetNextPowerOfTwo(required_buffer_size);
        bufferDesc.size = std::max(bufferDesc.size, uint64_t(65536));
        bufferDesc.usageBits = BUFFER_USAGE_TRANSFER_FROM_BIT;
        cmd.stageBuffer = m_Device->CreateBuffer(bufferDesc);

        m_Device->SetDebugName(cmd.stageBuffer, "CopyCmdAllocator staging buffer");
    }

    // Begin command buffer in valid state:
	VK_CHECK(vkResetCommandPool(m_Device->vkDevice, cmd.transferCmdPool, 0))
    VK_CHECK(vkResetCommandPool(m_Device->vkDevice, cmd.transitionCmdPool, 0))

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    VK_CHECK(vkBeginCommandBuffer(cmd.transferCmdBuffer, &beginInfo))
    VK_CHECK(vkBeginCommandBuffer(cmd.transitionCmdBuffer, &beginInfo))

    // Reset fence
    VK_CHECK(vkResetFences(m_Device->vkDevice, 1, &cmd.fence))
    return cmd;
}

void Device_Vulkan::CopyCmdAllocator::submit(CopyCmd cmd)
{
    VK_CHECK(vkEndCommandBuffer(cmd.transferCmdBuffer))
    VK_CHECK(vkEndCommandBuffer(cmd.transitionCmdBuffer))

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

    VkCommandBufferSubmitInfo cbSubmitInfo = {};
    cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cbSubmitInfo.commandBuffer = cmd.transferCmdBuffer;

    VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

    // Submit to transfer queue
    {
        cbSubmitInfo.commandBuffer = cmd.transferCmdBuffer;
        signalSemaphoreInfo.semaphore = cmd.semaphores[0]; // signal for graphics queue
        signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cbSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        std::scoped_lock lock(m_Device->m_queues[QUEUE_TYPE_ASYNC_TRANSFER].locker);
        m_Device->vkContext->extendFunction.pVkQueueSubmit2KHR(
            m_Device->m_queues[QUEUE_TYPE_ASYNC_TRANSFER].queue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    // Submit to graphics queue
    {
        waitSemaphoreInfo.semaphore = cmd.semaphores[0]; // wait for copy queue
        waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        cbSubmitInfo.commandBuffer = cmd.transitionCmdBuffer;
        signalSemaphoreInfo.semaphore = cmd.semaphores[1]; // signal for compute queue
        signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cbSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        std::scoped_lock lock(m_Device->m_queues[QUEUE_TYPE_GRAPHICS].locker);
        m_Device->vkContext->extendFunction.pVkQueueSubmit2KHR(
			m_Device->m_queues[QUEUE_TYPE_GRAPHICS].queue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    // insert semaphore to compute queue to make sure the copy and transition is done
    // this must be final submit in this function because it will also signal a fence for state tracking by CPU!
	{
		waitSemaphoreInfo.semaphore = cmd.semaphores[1]; // wait for graphics queue
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.commandBufferInfoCount = 0;
        submitInfo.pCommandBufferInfos = nullptr;
		submitInfo.signalSemaphoreInfoCount = 0;
		submitInfo.pSignalSemaphoreInfos = nullptr;

		std::scoped_lock lock(m_Device->m_queues[QUEUE_TYPE_ASYNC_COMPUTE].locker);
		m_Device->vkContext->extendFunction.pVkQueueSubmit2KHR(
			m_Device->m_queues[QUEUE_TYPE_ASYNC_COMPUTE].queue, 1, &submitInfo, cmd.fence);
	}

    std::scoped_lock lock(m_Locker);
    m_FreeList.push_back(cmd);
}

void Device_Vulkan::OnWindowResize(const WindowResizeEvent &event)
{
    m_frameBufferWidth = event.width;
    m_frameBufferHeight = event.height;
    m_recreateSwapchain = true;
    QK_CORE_LOGT_TAG("RHI", "Device_Vulkan hook window resize event. Width: {} Height: {}", m_frameBufferWidth, m_frameBufferHeight);
}

bool Device_Vulkan::Init()
{
    QK_CORE_LOGI_TAG("RHI", "==========Initializing Vulkan Backend...========");

    // Default values
    m_recreateSwapchain = false;
    m_elapsedFrame = 0;
    m_frameBufferWidth = Application::Get().GetWindow()->GetFrambufferWidth();
    m_frameBufferHeight = Application::Get().GetWindow()->GetFrambufferHeight();
    vkContext = CreateScope<VulkanContext>(); // TODO: Make configurable
    vkDevice = vkContext->logicalDevice; // Borrow from context
    vmaAllocator = vkContext->vmaAllocator;

    // Store device properties in public interface
    m_properties.limits.minUniformBufferOffsetAlignment = vkContext->properties2.properties.limits.minUniformBufferOffsetAlignment;
    m_features.textureCompressionBC = vkContext->features2.features.textureCompressionBC;
    m_features.textureCompressionASTC_LDR = vkContext->features2.features.textureCompressionASTC_LDR;;
    m_features.textureCompressionETC2 = vkContext->features2.features.textureCompressionETC2;
    
    // Create frame data
    for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; i++)
        m_frames[i].init(this);

    // Setup command queues
    m_queues[QUEUE_TYPE_GRAPHICS].init(this, QUEUE_TYPE_GRAPHICS);
    m_queues[QUEUE_TYPE_ASYNC_COMPUTE].init(this, QUEUE_TYPE_ASYNC_COMPUTE);
    m_queues[QUEUE_TYPE_ASYNC_TRANSFER].init(this, QUEUE_TYPE_ASYNC_TRANSFER);

    // Create Swapchain
    ResizeSwapchain();

    // Init copy cmds allocator
    copyAllocator.init(this);

    // Register callback functions
    EventManager::Get().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event);});
    QK_CORE_LOGI_TAG("RHI", "==========Vulkan Backend Initialized========");
    return true;
}

void Device_Vulkan::ShutDown()
{
    QK_CORE_LOGI_TAG("RHI", "Shutdown vulkan device...");
    vkDeviceWaitIdle(vkDevice);
    
    // Destroy cached pipeline layout
    cached_pipelineLayouts.clear();

    // Destory cached descriptor allocator
    cached_descriptorSetAllocator.clear();

    // Destroy copy allocator
    copyAllocator.destroy();

    // Destroy frames data
    for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; i++)
        m_frames[i].destroy();

    // Destroy vulkan context
    vkContext.reset();

}

bool Device_Vulkan::BeiginFrame(TimeStep ts)
{
    // move to next frame
    m_elapsedFrame++;

    // resize swapchain if needed. 
    if (m_recreateSwapchain) 
    {
        ResizeSwapchain();
        m_recreateSwapchain = false;
    }

    auto& frame = GetCurrentFrame();
    
    // wait for in-flight fences
    if (!frame.waitedFences.empty())
        vkWaitForFences(vkDevice, (uint32_t)frame.waitedFences.size(), frame.waitedFences.data(), true, 1000000000);
    
    // reset per frame data
    frame.reset();

    // put unused (more than 8 frames) descriptor set back to vacant pool
    for (auto& [k, value] : cached_descriptorSetAllocator)
        value.BeginFrame();   

    // Acquire a swapchain image 
    VkResult result = vkAcquireNextImageKHR(
        vkDevice,
        vkContext->swapChain,
        100000000,
        frame.imageAvailableSemaphore,
        nullptr,
        &m_currentSwapChainImageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        // Trigger swapchain recreation, then boot out of the render loop.
        ResizeSwapchain();
        return false;
    } 
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
    {
        QK_CORE_VERIFY(0, "Failed to acquire swapchain image!");
        return false;
    }

    return true;
}

bool Device_Vulkan::EndFrame(TimeStep ts)
{
    auto& frame = GetCurrentFrame();

    // Submit queued command lists with fence which would block the next next frame
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; ++i) 
    {
        if (frame.cmdListCount[i] > 0) 
        { // This queue is in use in this frame
            m_queues[i].submit(frame.queueFences[i]);
            frame.waitedFences.push_back(frame.queueFences[i]);
        }
    }

    // Prepare present
    VkSwapchainKHR swapchain = vkContext->swapChain;
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &frame.imageReleaseSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &m_currentSwapChainImageIdx;
	VkResult presentResult = vkQueuePresentKHR(vkContext->graphicQueue, &presentInfo);

    if (presentResult != VK_SUCCESS && presentResult != VK_ERROR_OUT_OF_DATE_KHR && presentResult != VK_SUBOPTIMAL_KHR)
        QK_CORE_ASSERT(0)

    return true;
}

Ref<Buffer> Device_Vulkan::CreateBuffer(const BufferDesc &desc, const void* initialData)
{
    return CreateRef<Buffer_Vulkan>(this, desc, initialData);
}

Ref<Image> Device_Vulkan::CreateImage(const ImageDesc &desc, const ImageInitData* init_data)
{
    QK_CORE_LOGT_TAG("RHI", "Vulkan image created");
    return CreateRef<Image_Vulkan>(this, desc, init_data);
}

Ref<Shader> Device_Vulkan::CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize)
{
    auto new_shader =  CreateRef<Shader_Vulkan>(this, stage, byteCode, codeSize);
    if (new_shader->GetShaderMoudule() == VK_NULL_HANDLE)
        return nullptr;

    return new_shader;
}

Ref<Shader> Device_Vulkan::CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path)
{
    std::vector<uint8_t> buffer;
    FileSystem::ReadFileBytes(file_path, buffer);

    auto new_shader = CreateShaderFromBytes(stage, buffer.data(), buffer.size());
    if (new_shader == nullptr) 
    {
        QK_CORE_LOGE_TAG("RHI", "Faile to create shader from file: {}", file_path);
        return nullptr;
    }

    return new_shader;
}

Ref<PipeLine> Device_Vulkan::CreateGraphicPipeLine(const GraphicPipeLineDesc &desc)
{
    Ref<PipeLine> newPso = CreateRef<PipeLine_Vulkan>(this, desc);
    QK_CORE_LOGT_TAG("RHI", "Graphic Pipeline created");

    return newPso;
}

Ref<Sampler> Device_Vulkan::CreateSampler(const SamplerDesc &desc)
{
    Ref<Sampler> newSamper = CreateRef<Sampler_Vulkan>(this, desc);
    QK_CORE_LOGT_TAG("RHI", "Vulkan sampler Created");

    return newSamper;
}

void Device_Vulkan::SetDebugName(const Ref<GpuResource>& resouce, const char* name)
{
    if (!vkContext->enableDebugUtils || !resouce)
        return;


    VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    info.pObjectName = name;

    switch (resouce->GetGpuResourceType())
    {
    case GpuResourceType::BUFFER:
        info.objectType = VK_OBJECT_TYPE_BUFFER;
        info.objectHandle = (uint64_t)static_cast<Buffer_Vulkan*>(resouce.get())->GetHandle();
		break;
    case GpuResourceType::IMAGE:
		info.objectType = VK_OBJECT_TYPE_IMAGE;
        info.objectHandle = (uint64_t)static_cast<Image_Vulkan*>(resouce.get())->GetHandle();
        break;
    default:
        break;
    }

    if (info.objectHandle == (uint64_t)VK_NULL_HANDLE)
        return;

    PFN_vkSetDebugUtilsObjectNameEXT pfnVkSetDebugUtilsObjectNameEXT =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vkContext->instance, "vkSetDebugUtilsObjectNameEXT");

    VK_CHECK(pfnVkSetDebugUtilsObjectNameEXT(vkDevice, &info))
}

CommandList* Device_Vulkan::BeginCommandList(QueueType type)
{
    auto& frame = GetCurrentFrame();
    auto& cmdLists = frame.cmdLists[type];
    u32 cmd_count = frame.cmdListCount[type]++;
    if (cmd_count >= cmdLists.size()) 
    {
        // Create a new Command list is needed
        cmdLists.emplace_back(new CommandList_Vulkan(this, type));
    }

    CommandList_Vulkan* internal_cmdList = cmdLists[cmd_count];
    internal_cmdList->ResetAndBeginCmdBuffer();
    
    return static_cast<CommandList*>(internal_cmdList);
}

void Device_Vulkan::SubmitCommandList(CommandList* cmd, CommandList* waitedCmds, uint32_t waitedCmdCounts, bool signal)
{
    auto& internal_cmdList = ToInternal(cmd);
    auto& queue = m_queues[internal_cmdList.GetQueueType()];

    vkEndCommandBuffer(internal_cmdList.GetHandle());
    internal_cmdList.state = CommandListState::READY_FOR_SUBMIT;
    
    if (queue.submissions.empty()) 
    {
        queue.submissions.emplace_back();
    }

    // The signalSemaphoreInfos should always be empty
    if (!queue.submissions.back().signalSemaphoreInfos.empty() || !queue.submissions.back().waitSemaphoreInfos.empty()) 
    {
        // Need to create a new batch
        queue.submissions.emplace_back();
    }

    auto& submission = queue.submissions.back();
    VkCommandBufferSubmitInfo& cmd_submit_info = submission.cmdInfos.emplace_back();
    cmd_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submit_info.commandBuffer = internal_cmdList.GetHandle();
    cmd_submit_info.pNext = nullptr;

    if (waitedCmdCounts > 0) 
    {
        for (size_t i = 0; i < waitedCmdCounts; ++i) 
        {
            auto& internal = ToInternal(&waitedCmds[i]);
            auto& semaphore_info = submission.waitSemaphoreInfos.emplace_back();
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            semaphore_info.semaphore = internal.GetCmdCompleteSemaphore();
            semaphore_info.value = 0; // not a timeline semaphore
            semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Perfomance trade off here for abstracting away semaphore

        }
    }

    auto& frame = GetCurrentFrame();
    if (internal_cmdList.IsWaitingForSwapChainImage() && !frame.imageAvailableSemaphoreConsumed) 
    {
        auto& wait_semaphore_info = submission.waitSemaphoreInfos.emplace_back();
        wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait_semaphore_info.semaphore = frame.imageAvailableSemaphore;
        wait_semaphore_info.value = 0;
        wait_semaphore_info.stageMask = internal_cmdList.GetSwapChainWaitStages();

        // TODO: This indicates there is only one command buffer in a frame can manipulate swapchain images. Not flexible enough
        auto& submit_semaphore_info = submission.signalSemaphoreInfos.emplace_back();
        submit_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_semaphore_info.semaphore = frame.imageReleaseSemaphore;

        frame.imageAvailableSemaphoreConsumed = true;
    }

    // Submit right now to make sure the correct order of submissions
    if (signal) 
    {
        auto& semaphore_info = submission.signalSemaphoreInfos.emplace_back();
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        semaphore_info.semaphore = internal_cmdList.GetCmdCompleteSemaphore();
        semaphore_info.value = 0; // not a timeline semaphore
        semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        queue.submit();
    }
}

void Device_Vulkan::ResizeSwapchain()
{
    QK_CORE_LOGI_TAG("RHI", "Resizing swapchain...");
    
    vkContext->DestroySwapChain();
    vkContext->CreateSwapChain();

    m_swapChainImages.clear();
    for (size_t i = 0; i < vkContext->swapChianImages.size(); i++) {
        ImageDesc desc;
        desc.type = ImageType::TYPE_2D;
        desc.height = vkContext->swapChainExtent.height;
        desc.width = vkContext->swapChainExtent.width;
        desc.depth = 1;
        desc.format = GetPresentImageFormat();

        Ref<Image> newImage = CreateRef<Image_Vulkan>(desc);
        auto& internal_image = ToInternal(newImage.get());
        internal_image.m_Device = this;
        internal_image.m_Handle = vkContext->swapChianImages[i];
        internal_image.m_View = vkContext->swapChainImageViews[i];
        internal_image.m_IsSwapChainImage = true;
        m_swapChainImages.push_back(newImage);
    }

}

DataFormat Device_Vulkan::GetPresentImageFormat()
{
    VkFormat format = vkContext->surfaceFormat.format;

    switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
        return DataFormat::R8G8B8A8_UNORM;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return DataFormat::R16G16B16A16_SFLOAT;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return DataFormat::B8G8R8A8_UNORM;
    default:
        QK_CORE_VERIFY(0, "format not handled yet")
        return DataFormat::UNDEFINED;
    }
}

PipeLineLayout* Device_Vulkan::Request_PipeLineLayout(const ShaderResourceLayout& combinedLayout)
{
    // Compute pipeline layout hash
    size_t hash = 0;
    for (size_t i = 0; i < DESCRIPTOR_SET_MAX_NUM; ++i) {
        for (size_t j = 0; j < SET_BINDINGS_MAX_NUM; ++j) {
            auto& binding = combinedLayout.descriptorSetLayouts[i].vk_bindings[j];
            util::hash_combine(hash, binding.binding);
            util::hash_combine(hash, binding.descriptorCount);
            util::hash_combine(hash, binding.descriptorType);
            util::hash_combine(hash, binding.stageFlags);
            // util::hash_combine(hash, pipeline_layout.imageViewTypes[i][j]);
        }
    }
    util::hash_combine(hash, combinedLayout.pushConstant.offset);
    util::hash_combine(hash, combinedLayout.pushConstant.size);
    util::hash_combine(hash, combinedLayout.pushConstant.stageFlags);
    util::hash_combine(hash, combinedLayout.descriptorSetLayoutMask);

    auto find = cached_pipelineLayouts.find(hash);
    if (find == cached_pipelineLayouts.end()) {
        // need to create a new pipeline layout
        auto result = cached_pipelineLayouts.try_emplace(hash, this, combinedLayout);
        return &(result.first->second);
    }
    else {
        return &find->second;
    }
}

bool Device_Vulkan::isFormatSupported(DataFormat format)
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(vkContext->physicalDevice, ConvertDataFormat(format), &props);
    return ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) && (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT));
}

DescriptorSetAllocator* Device_Vulkan::Request_DescriptorSetAllocator(const DescriptorSetLayout& layout)
{
    size_t hash = 0;
    util::hash_combine(hash, layout.uniform_buffer_mask);
    util::hash_combine(hash, layout.storage_buffer_mask);
    util::hash_combine(hash, layout.sampled_image_mask);
    util::hash_combine(hash, layout.storage_image_mask);
    util::hash_combine(hash, layout.separate_image_mask);
    util::hash_combine(hash, layout.sampler_mask);
    util::hash_combine(hash, layout.input_attachment_mask);

    auto find = cached_descriptorSetAllocator.find(hash);
    if (find == cached_descriptorSetAllocator.end()) {
        // need to create a new descriptor set allocator
        auto ret = cached_descriptorSetAllocator.try_emplace(hash, this, layout);
        return &(ret.first->second);
    }
    else {
        return &find->second;
    }

}
}

