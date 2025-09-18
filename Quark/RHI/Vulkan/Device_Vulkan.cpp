#include "Quark/qkpch.h"
#define VMA_IMPLEMENTATION
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Math/Util.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Events/EventManager.h"
#include "Quark/RHI/Vulkan/Shader_Vulkan.h"

#define LOCK() std::lock_guard<std::mutex> _holder_##__COUNTER__{m_lock.lock}
#define LOCK_CACHE() std::lock_guard<std::mutex> _holder_##__COUNTER__{m_lock.read_only_cache_lock} //TODO: make this a read - write lock
#define DRAIN_FRAME_LOCK() \
	std::unique_lock<std::mutex> _holder{m_lock.lock}; \
	m_lock.cond.wait(_holder, [&]() { \
		return m_lock.counter == 0; \
	})

namespace quark::rhi {

static void request_block(Device& device, BufferBlock& block, VkDeviceSize size,
    BufferPool& pool, std::vector<BufferBlock>& recycle)
{
    if (block.GetOffset() == 0)
    {
        if (block.GetSize() == pool.GetBlockSize())
            pool.RecycleBlock(block);
    }
    else
    {
        if (block.GetSize() == pool.GetBlockSize())
            recycle.push_back(block);
    }

    if (size)
        block = pool.RequestBlock(size);
    else
        block = {};
}

void Device_Vulkan::WindowSystemIntergration::init(Device_Vulkan* _device)
{
    device = _device;
    swapchain_images.resize(device->GetVulkanContext().swapChianImages.size());

    // create semaphores
    VkSemaphoreCreateInfo semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphore_create_info.pNext = nullptr;
    uint8_t swapchain_image_count = (uint8_t)device->GetVulkanContext().swapChianImages.size();
    acquire_semaphores.resize(swapchain_image_count);
    release_semaphores.resize(swapchain_image_count);
    for (uint8_t i = 0; i < swapchain_image_count; i++)
    {
        vkCreateSemaphore(device->vkDevice, &semaphore_create_info, nullptr, &acquire_semaphores[i]);
        vkCreateSemaphore(device->vkDevice, &semaphore_create_info, nullptr, &release_semaphores[i]);
    }
}

void Device_Vulkan::WindowSystemIntergration::destroy()
{
    // destroy semaphores
    for (uint8_t i = 0; i < (uint8_t)device->GetVulkanContext().swapChianImages.size(); i++)
	{
		vkDestroySemaphore(device->vkDevice, acquire_semaphores[i], nullptr);
		vkDestroySemaphore(device->vkDevice, release_semaphores[i], nullptr);
	}
}
void Device_Vulkan::CommandQueue::init(Device_Vulkan *device, QueueType type)
{
    this->device = device;
    this->type = type;

    switch (type) {
    case QUEUE_TYPE_GRAPHICS:
        queue = device->GetVulkanContext().graphicQueue;
        break;
    case QUEUE_TYPE_ASYNC_COMPUTE:
        queue = device->GetVulkanContext().computeQueue;
        break;
    case QUEUE_TYPE_ASYNC_TRANSFER:
        queue = device->GetVulkanContext().transferQueue;
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

    device->GetVulkanContext().extendFunction.pVkQueueSubmit2KHR(queue, (uint32_t)submit_infos.size(), submit_infos.data(), fence);
    
    // Clear submissions
    for(auto& submission : submissions)
    {
        submission.cmdInfos.clear();
        submission.signalSemaphoreInfos.clear();
        submission.waitSemaphoreInfos.clear();
    }

    submissions.clear();
}

void PerFrameContext::init(Device_Vulkan *device)
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
   
}

void PerFrameContext::clear()
{
    VkDevice vk_device = device->vkDevice;

    // destroy deferred destroyed resources
    for (auto& sampler : garbage_samplers) 
        vkDestroySampler(vk_device, sampler, nullptr);
    for (auto& view : grabage_views) 
        vkDestroyImageView(vk_device, view, nullptr);
    for (auto& buffer : garbage_buffers) 
        vmaDestroyBuffer(vmaAllocator, buffer.first, buffer.second);
    for (auto& image : garbage_images) 
        vmaDestroyImage(vmaAllocator, image.first, image.second);
    for (auto& pipeline : garbage_pipelines) 
        vkDestroyPipeline(vk_device, pipeline, nullptr);
    for (auto& shaderModule_ : garbage_shaderModules)
        vkDestroyShaderModule(vk_device, shaderModule_, nullptr);

    garbage_samplers.clear();
    garbage_buffers.clear();
    grabage_views.clear();
    garbage_images.clear();
    garbage_pipelines.clear();
    garbage_shaderModules.clear();
}

void PerFrameContext::begin()
{
    // wait for in-flight fences
    if (!waitedFences.empty())
        vkWaitForFences(device->vkDevice, (uint32_t)waitedFences.size(), waitedFences.data(), true, UINT64_MAX);

    if (!waitedFences.empty()) 
    {
        vkResetFences(device->vkDevice, (uint32_t)waitedFences.size(), waitedFences.data());
        waitedFences.clear();
    }

    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++)
        cmdListCount[i] = 0;

    // recycle buffer blocks
    for (auto& b : ubo_blocks)
        device->m_ubo_pool.RecycleBlock(b);
    for (auto& b : vbo_blocks)
        device->m_vbo_pool.RecycleBlock(b);
    for (auto& b : staging_blocks)
        device->m_staging_pool.RecycleBlock(b);

    // clear un-recycled blocks, the internal buffer will be collected as garbage
    ubo_blocks.clear();
    vbo_blocks.clear();
    staging_blocks.clear();

    // destroy deferred-destroyed resources
    clear();
}

void PerFrameContext::destroy()
{
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) 
    {
        for (size_t j = 0; j < cmdLists[i].size(); j++) 
        {
            if (cmdLists[i][j] != nullptr)
            {
                delete cmdLists[i][j];
                cmdLists[i][j] = nullptr;
            }
        }
        cmdLists[i].clear();

        vkDestroyFence(device->vkDevice, queueFences[i], nullptr);
    }

    ubo_blocks.clear();
    vbo_blocks.clear();
    staging_blocks.clear();
    
    clear();
}

void CopyCmdAllocator::init(Device_Vulkan *device)
{
    m_device = device;
}

void CopyCmdAllocator::destroy()
{   
    // Make sure all allocated cmd are in free list
    vkQueueWaitIdle(m_device->m_queues[QUEUE_TYPE_ASYNC_TRANSFER].queue);
    for (auto& x : m_freeList)
    {
        vkDestroyCommandPool(m_device->vkDevice, x.transferCmdPool, nullptr);
        vkDestroyCommandPool(m_device->vkDevice, x.transitionCmdPool, nullptr);
        vkDestroyFence(m_device->vkDevice, x.fence, nullptr);
        vkDestroySemaphore(m_device->vkDevice, x.semaphores[0], nullptr);
        vkDestroySemaphore(m_device->vkDevice, x.semaphores[1], nullptr);
    }

    m_freeList.clear();
}

CopyCmdAllocator::CopyCmd CopyCmdAllocator::allocate(VkDeviceSize required_buffer_size)
{
    CopyCmd cmd;

    // Try to find a suitable staging buffer in free list
    m_locker.lock();
    for (size_t i = 0; i < m_freeList.size(); ++i) 
    {
        if (m_freeList[i].stageBuffer->GetDesc().size >= required_buffer_size) 
        {
            if (vkGetFenceStatus(m_device->vkDevice, m_freeList[i].fence) == VK_SUCCESS)
            {
                cmd = std::move(m_freeList[i]);
                std::swap(m_freeList[i], m_freeList.back());
                m_freeList.pop_back();
                break;
            }
        }
    }
    m_locker.unlock();

    if (!cmd.isValid()) // No suitable staging buffer founded
    { 
        // Create command pool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_device->GetVulkanContext().transferQueueIndex;
        VK_CHECK(vkCreateCommandPool(m_device->vkDevice, &poolInfo, nullptr, &cmd.transferCmdPool))

        poolInfo.queueFamilyIndex = m_device->GetVulkanContext().graphicQueueIndex;
        VK_CHECK(vkCreateCommandPool(m_device->vkDevice, &poolInfo, nullptr, &cmd.transitionCmdPool))

        // Allocate command buffer
        VkCommandBufferAllocateInfo commandBufferInfo = {};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferInfo.commandBufferCount = 1;
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandPool = cmd.transferCmdPool;
        VK_CHECK(vkAllocateCommandBuffers(m_device->vkDevice, &commandBufferInfo, &cmd.transferCmdBuffer))

        commandBufferInfo.commandPool = cmd.transitionCmdPool;
        VK_CHECK(vkAllocateCommandBuffers(m_device->vkDevice, &commandBufferInfo, &cmd.transitionCmdBuffer))

        // Create fence
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VK_CHECK(vkCreateFence(m_device->vkDevice, &fenceInfo, nullptr, &cmd.fence))

        // Create Semaphores
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK(vkCreateSemaphore(m_device->vkDevice, &semaphoreInfo, nullptr, &cmd.semaphores[0]));
        VK_CHECK(vkCreateSemaphore(m_device->vkDevice, &semaphoreInfo, nullptr, &cmd.semaphores[1]));

        // Create staging buffer
        BufferDesc bufferDesc;
        bufferDesc.domain = BufferMemoryDomain::CPU;
        bufferDesc.size = math::GetNextPowerOfTwo(required_buffer_size);
        bufferDesc.size = std::max(bufferDesc.size, uint64_t(65536));
        bufferDesc.usageBits = BUFFER_USAGE_TRANSFER_FROM_BIT;
        cmd.stageBuffer = m_device->CreateBuffer(bufferDesc);
        auto& internal = ToInternal(cmd.stageBuffer.get());
        internal.SetInternalSynced();
        m_device->SetName(cmd.stageBuffer, "CopyCmdAllocator staging buffer");
    }

    // Begin command buffer in valid state:
	VK_CHECK(vkResetCommandPool(m_device->vkDevice, cmd.transferCmdPool, 0))
    VK_CHECK(vkResetCommandPool(m_device->vkDevice, cmd.transitionCmdPool, 0))

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    VK_CHECK(vkBeginCommandBuffer(cmd.transferCmdBuffer, &beginInfo))
    VK_CHECK(vkBeginCommandBuffer(cmd.transitionCmdBuffer, &beginInfo))

    // Reset fence
    VK_CHECK(vkResetFences(m_device->vkDevice, 1, &cmd.fence))
    return cmd;
}

void CopyCmdAllocator::submit(CopyCmd cmd)
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

        std::scoped_lock lock(m_device->m_queues[QUEUE_TYPE_ASYNC_TRANSFER].locker);
        m_device->GetVulkanContext().extendFunction.pVkQueueSubmit2KHR(
            m_device->m_queues[QUEUE_TYPE_ASYNC_TRANSFER].queue, 1, &submitInfo, VK_NULL_HANDLE);
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

        std::scoped_lock lock(m_device->m_queues[QUEUE_TYPE_GRAPHICS].locker);
        m_device->GetVulkanContext().extendFunction.pVkQueueSubmit2KHR(
			m_device->m_queues[QUEUE_TYPE_GRAPHICS].queue, 1, &submitInfo, VK_NULL_HANDLE);
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

		std::scoped_lock lock(m_device->m_queues[QUEUE_TYPE_ASYNC_COMPUTE].locker);
		m_device->GetVulkanContext().extendFunction.pVkQueueSubmit2KHR(
			m_device->m_queues[QUEUE_TYPE_ASYNC_COMPUTE].queue, 1, &submitInfo, cmd.fence);
	}

    std::scoped_lock lock(m_locker);
    m_freeList.push_back(cmd);
}

void Device_Vulkan::OnWindowResize(const WindowResizeEvent &event)
{
    m_frameBufferWidth = event.width;
    m_frameBufferHeight = event.height;
    m_wsi.recreate_swapchain = true;
    QK_CORE_LOGT_TAG("RHI", "Device_Vulkan hook window resize event. Width: {} Height: {}", m_frameBufferWidth, m_frameBufferHeight);
}

Device_Vulkan::Device_Vulkan(const DeviceConfig& config)
{
    QK_CORE_LOGI_TAG("RHI", "Creating Device_Vulkan...");

    // setup default values
    QK_CORE_ASSERT(config.framesInFlight <= MAX_FRAME_NUM_IN_FLIGHT);
    m_config = config;
    m_wsi.recreate_swapchain = false;
    m_frameBufferWidth = Application::Get().GetWindow()->GetFrambufferWidth();
    m_frameBufferHeight = Application::Get().GetWindow()->GetFrambufferHeight();
    m_vulkan_context = CreateScope<VulkanContext>(); // TODO: Make configurable
    vkDevice = m_vulkan_context->logicalDevice; // Borrow from context
    vmaAllocator = m_vulkan_context->vmaAllocator;

    // store device properties in public interface
    m_properties.limits.minUniformBufferOffsetAlignment = m_vulkan_context->gpu_properties2.properties.limits.minUniformBufferOffsetAlignment;

    // create per-frame data
    m_frames.resize(config.framesInFlight);
    for (uint8_t i = 0; i < config.framesInFlight; i++)
        m_frames[i].init(this);

    // setup command queues
    m_queues[QUEUE_TYPE_GRAPHICS].init(this, QUEUE_TYPE_GRAPHICS);
    m_queues[QUEUE_TYPE_ASYNC_COMPUTE].init(this, QUEUE_TYPE_ASYNC_COMPUTE);
    m_queues[QUEUE_TYPE_ASYNC_TRANSFER].init(this, QUEUE_TYPE_ASYNC_TRANSFER);

    // init wsi and swapchain
    m_wsi.init(this);
    ResizeSwapchain();
    QK_CORE_LOGT_TAG("RHI", "Wsi initialized");

    // init copy cmds allocator
    copyAllocator.init(this);

    // init buffer pools
    m_vbo_pool.Init(this, 4 * 1024, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_ubo_pool.Init(this, 256 * 1024, std::max<VkDeviceSize>(16u, GetDeviceProperties().limits.minUniformBufferOffsetAlignment)
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    m_staging_pool.Init(this, 64 * 1024, std::max<VkDeviceSize>(m_vulkan_context->gpu_properties2.properties.limits.minStorageBufferOffsetAlignment,
        std::max<VkDeviceSize>(16u, m_vulkan_context->gpu_properties2.properties.limits.optimalBufferCopyOffsetAlignment)),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    m_ubo_pool.SetSpillRegionSize(VULKAN_MAX_UBO_SIZE);
    m_ubo_pool.SetMaxRetainedBlocks(64);
    m_vbo_pool.SetMaxRetainedBlocks(256);
    m_staging_pool.SetMaxRetainedBlocks(32);

    // register callback functions
    EventManager::Get().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event); });
    
    QK_CORE_LOGI_TAG("RHI", "Device_Vulkan Created");
}

Device_Vulkan::~Device_Vulkan()
{
    QK_CORE_LOGI_TAG("RHI", "Shutdown vulkan device...");

    DRAIN_FRAME_LOCK();
    if (!m_frames.empty())
        EndFrameContextNoLock();
    vkDeviceWaitIdle(vkDevice);

    // destroy cached pipeline layout
    cached_pipelineLayouts.clear();
    // destory cached descriptor allocator
    cached_descriptorSetAllocator.clear();

    // destroy copy allocator
    copyAllocator.destroy();

    m_ubo_pool.Reset();
    m_vbo_pool.Reset();
    m_staging_pool.Reset();

    // destroy command buffers, pools, semaphore, and fences, deferred garbage resouces
    for (size_t i = 0; i < m_frames.size(); i++)
        m_frames[i].destroy();

    // destroy command list will push some garbage buffers to the current frame
    GetCurrentFrame().clear();

    // destroy wsi data
    m_wsi.destroy();

    // destroy vulkan context
    m_vulkan_context.reset();
}

const DeviceFeatures& Device_Vulkan::GetDeviceFeatures() const
{
    return *static_cast<DeviceFeatures*>(&m_vulkan_context->deviceFeatures);
}

void Device_Vulkan::NextFrameContext()
{
    DRAIN_FRAME_LOCK();

    // Flush the frame here as we might have pending staging command buffers from init stage.
    EndFrameContextNoLock();

    // put unused (more than 8 frames) descriptor set back to vacant pool
    for (auto& [k, value] : cached_descriptorSetAllocator)
        value.BeginFrame();

    // move to next frame
    m_frame_context_index++;
    if (m_frame_context_index >= m_config.framesInFlight)
        m_frame_context_index = 0;

    GetCurrentFrame().begin();
}

bool Device_Vulkan::BeiginFrame(TimeStep ts)
{

    NextFrameContext();

    // resize swapchain if needed. 
    if (m_wsi.recreate_swapchain) 
    {
        ResizeSwapchain();
        m_wsi.recreate_swapchain = false;
    }
    
    
    // reset wsi
    m_wsi.consumed = false;
    m_wsi.semaphore_index++;
    if (m_wsi.semaphore_index >= m_wsi.acquire_semaphores.size())
		m_wsi.semaphore_index = 0;


    // Acquire a swapchain image 
    VkResult result = vkAcquireNextImageKHR(
        vkDevice,
        m_vulkan_context->swapChain,
        100000000,
        m_wsi.acquire_semaphores[m_wsi.semaphore_index],
        nullptr,
        &m_wsi.swapchain_image_index);

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

    EndFrameContext();

    // Prepare present
    VkSwapchainKHR swapchain = m_vulkan_context->swapChain;
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &m_wsi.release_semaphores[m_wsi.semaphore_index];
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &m_wsi.swapchain_image_index;
	VkResult presentResult = vkQueuePresentKHR(m_vulkan_context->graphicQueue, &presentInfo);

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

Ref<ImageView> Device_Vulkan::CreateImageView(const ImageViewDesc& desc)
{
    return CreateRef<ImageView_Vulkan>(this, desc);
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

void Device_Vulkan::CopyBuffer(Buffer& dst, Buffer& src, uint64_t size, uint64_t dstOffset, uint64_t srcOffset)
{
    auto& dst_internal = ToInternal(&dst);
    auto& src_internal = ToInternal(&src);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;

    CopyCmdAllocator::CopyCmd copyCmd = copyAllocator.allocate(0);
    vkCmdCopyBuffer(copyCmd.transferCmdBuffer, src_internal.GetHandle(), dst_internal.GetHandle(), 1, &copyRegion);
    copyAllocator.submit(copyCmd);
}

void Device_Vulkan::WaitIdle()
{
    DRAIN_FRAME_LOCK();

    if (vkDevice != VK_NULL_HANDLE)
    {
        auto result = vkDeviceWaitIdle(vkDevice);
        if (result != VK_SUCCESS)
            QK_CORE_LOGE_TAG("RHI", "vkDeviceWaitIdle failed with code: {}", (int)result);
    }

    // free memory for buffer pools
    m_ubo_pool.Reset();
    m_vbo_pool.Reset();
    m_staging_pool.Reset();

    for (auto& frame : m_frames)
    {
        frame.ubo_blocks.clear();
        frame.vbo_blocks.clear();
        frame.staging_blocks.clear();
    }

    for (auto& frame : m_frames)
        frame.clear();

}

void Device_Vulkan::SetName(const Ref<GpuResource>& resouce, const char* name)
{
    if (!m_vulkan_context->supportDebugUtils || !resouce)
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

    VK_CHECK(m_vulkan_context->extendFunction.pVkSetDebugUtilsObjectNameEXT(vkDevice, &info))
}

CommandList* Device_Vulkan::BeginCommandList(QueueType type)
{
    LOCK();
    return RequestCommandListNoLock(type);
}

void Device_Vulkan::SubmitCommandList(CommandList* cmd, CommandList* waitedCmds, uint32_t waitedCmdCounts, bool signal)
{
    LOCK();
    SubmitCommandListNoLock(cmd, waitedCmds, waitedCmdCounts, signal);
}

uint32_t Device_Vulkan::AllocateCookie()
{
    // reserve lower bits for "special purposes".
    return m_cookie.fetch_add(16, std::memory_order_relaxed) + 16;
}

void Device_Vulkan::DestroyBufferNoLock(VkBuffer buffer, VmaAllocation alloc)
{
    GetCurrentFrame().garbage_buffers.push_back({ buffer, alloc });
}

void Device_Vulkan::DestroyBuffer(VkBuffer buffer, VmaAllocation alloc)
{
    LOCK();
    DestroyBufferNoLock(buffer, alloc);
}

void Device_Vulkan::DestroyImageNoLock(VkImage image, VmaAllocation alloc)
{
    GetCurrentFrame().garbage_images.push_back({ image, alloc});
}

void Device_Vulkan::DestroyImage(VkImage image, VmaAllocation alloc)
{
    LOCK();
	DestroyImageNoLock(image, alloc);
}

void Device_Vulkan::DestroyImageViewNoLock(VkImageView view)
{
    GetCurrentFrame().grabage_views.push_back(view);
}

void Device_Vulkan::DestroyImageView(VkImageView view)
{
    LOCK();
	DestroyImageViewNoLock(view);
}

void Device_Vulkan::ResizeSwapchain()
{
    QK_CORE_LOGI_TAG("RHI", "Resizing swapchain...");
    
    m_vulkan_context->DestroySwapChain();
    m_vulkan_context->CreateSwapChain();

    QK_CORE_ASSERT(m_wsi.swapchain_images.size() > 0);
    m_wsi.swapchain_images.clear();
    for (size_t i = 0; i < m_vulkan_context->swapChianImages.size(); i++) {
        ImageDesc desc;
        desc.type = ImageType::TYPE_2D;
        desc.height = m_vulkan_context->swapChainExtent.height;
        desc.width = m_vulkan_context->swapChainExtent.width;
        desc.depth = 1;
        desc.format = GetPresentImageFormat();
        ImageViewDesc view_desc;
        view_desc.viewType = ImageViewType::TYPE_2D;
        view_desc.format = desc.format;
        view_desc.baseLayer = 0;
        view_desc.baseLevel = 0;
        view_desc.levelCount = 1;
        view_desc.layerCount = 1;
        view_desc.aspect = IMAGE_ASPECT_COLOR_BIT;

        Ref<Image> newImage = CreateRef<Image_Vulkan>(this, desc);
        view_desc.image = newImage.get();
        Ref<ImageView_Vulkan> newView = CreateRef<ImageView_Vulkan>(this, m_vulkan_context->swapChainImageViews[i], view_desc);

        auto& internal_image = ToInternal(newImage.get());
        internal_image.m_handle = m_vulkan_context->swapChianImages[i];
        internal_image.m_default_view = newView;
        internal_image.m_isSwapChainImage = true;
        // m_swapChainImages.push_back(newImage);
        m_wsi.swapchain_images.push_back(newImage);
    }

}

void Device_Vulkan::AddFrameCounterNoLock()
{
    m_lock.counter++;
}

void Device_Vulkan::DecrementFrameCounterNoLock()
{
    QK_CORE_ASSERT(m_lock.counter > 0);
    m_lock.counter--;
    m_lock.cond.notify_all();
}

void Device_Vulkan::SubmitCommandListNoLock(CommandList* cmd, CommandList* waitedCmds, uint32_t waitedCmdCounts, bool signal)
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
    if (internal_cmdList.IsWaitingForSwapChainImage() && !m_wsi.consumed)
    {
        auto& wait_semaphore_info = submission.waitSemaphoreInfos.emplace_back();
        wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait_semaphore_info.semaphore = m_wsi.acquire_semaphores[m_wsi.semaphore_index];
        wait_semaphore_info.value = 0;
        wait_semaphore_info.stageMask = internal_cmdList.GetSwapChainWaitStages();

        // TODO: This indicates there is only one command buffer in a frame can manipulate swapchain images. Not flexible enough
        auto& submit_semaphore_info = submission.signalSemaphoreInfos.emplace_back();
        submit_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_semaphore_info.semaphore = m_wsi.release_semaphores[m_wsi.semaphore_index];

        m_wsi.consumed = true;
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

    DecrementFrameCounterNoLock();
}

void Device_Vulkan::EndFrameContext()
{
    DRAIN_FRAME_LOCK();
    EndFrameContextNoLock();
}

void Device_Vulkan::EndFrameContextNoLock()
{
    auto& frame = GetCurrentFrame();

    // Submit queued command lists with fence which would block the next next frame
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; ++i)
    {
        if (frame.cmdListCount[i] > 0 && !m_queues[i].submissions.empty())
        { // This queue is in use in this frame
            m_queues[i].submit(frame.queueFences[i]);
            frame.waitedFences.push_back(frame.queueFences[i]);
        }
    }
}

CommandList* Device_Vulkan::RequestCommandListNoLock(QueueType type)
{
    auto& frame = GetCurrentFrame();
    auto& cmdLists = frame.cmdLists[type];
    uint32_t cmd_count = frame.cmdListCount[type]++;
    if (cmd_count >= cmdLists.size())
    {
        // Create a new Command list is needed
        cmdLists.emplace_back(new CommandList_Vulkan(this, type));
    }

    CommandList_Vulkan* internal_cmdList = cmdLists[cmd_count];
    internal_cmdList->ResetAndBeginCmdBuffer();
    AddFrameCounterNoLock();
    return static_cast<CommandList*>(internal_cmdList);
}

DataFormat Device_Vulkan::GetPresentImageFormat()
{
    VkFormat format = m_vulkan_context->surfaceFormat.format;

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

PipeLineLayout* Device_Vulkan::RequestPipeLineLayout(const CombinedResourceLayout& combinedLayout)
{
    // Compute pipeline layout hash
    size_t hash = 0;
    for (size_t i = 0; i < DESCRIPTOR_SET_MAX_NUM; ++i) {
        for (size_t j = 0; j < SET_BINDINGS_MAX_NUM; ++j) {
            auto& binding = combinedLayout.descriptor_set_layouts[i].vk_bindings[j];
            util::hash_combine(hash, binding.binding);
            util::hash_combine(hash, binding.descriptorCount);
            util::hash_combine(hash, binding.descriptorType);
            util::hash_combine(hash, binding.stageFlags);
            // util::hash_combine(hash, pipeline_layout.imageViewTypes[i][j]);
        }
    }
    util::hash_combine(hash, combinedLayout.push_constant_range.offset);
    util::hash_combine(hash, combinedLayout.push_constant_range.size);
    util::hash_combine(hash, combinedLayout.push_constant_range.stageFlags);
    util::hash_combine(hash, combinedLayout.descriptor_set_mask);

    // LOCK_CACHE();
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

void Device_Vulkan::RequestUniformBlock(BufferBlock& block, VkDeviceSize size)
{
    LOCK();
    RequestUniformBlockNoLock(block, size);
}

void Device_Vulkan::RequestUniformBlockNoLock(BufferBlock& block, VkDeviceSize size)
{
    request_block(*this, block, size, m_ubo_pool, GetCurrentFrame().ubo_blocks);
}

void Device_Vulkan::RequestVertexBlock(BufferBlock& block, VkDeviceSize size)
{
    LOCK();
    RequestVertexBlockNoLock(block, size);
}

void Device_Vulkan::RequestVertexBlockNoLock(BufferBlock& block, VkDeviceSize size)
{
    request_block(*this, block, size, m_vbo_pool, GetCurrentFrame().vbo_blocks);
}

void Device_Vulkan::RequestStagingBlock(BufferBlock& block, VkDeviceSize size)
{
    LOCK();
    RequestStagingBlockNoLock(block, size);
}

void Device_Vulkan::RequestStagingBlockNoLock(BufferBlock& block, VkDeviceSize size)
{
    request_block(*this, block, size, m_staging_pool, GetCurrentFrame().staging_blocks);
}

void Device_Vulkan::GetFormatProperties(VkFormat format, VkFormatProperties3* properties3) const
{
    VkFormatProperties2 properties2 = { VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
    QK_CORE_ASSERT(properties3->sType == VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3);

    if (m_vulkan_context->deviceFeatures.supports_format_feature_flags2)
    {
        properties2.pNext = properties3;
        vkGetPhysicalDeviceFormatProperties2(m_vulkan_context->physicalDevice, format, &properties2);
    }
    else
    {
        // Skip properties3 and synthesize the results instead.
        properties2.pNext = properties3->pNext;
        vkGetPhysicalDeviceFormatProperties2(m_vulkan_context->physicalDevice, format, &properties2);

        properties3->optimalTilingFeatures = properties2.formatProperties.optimalTilingFeatures;
        properties3->linearTilingFeatures = properties2.formatProperties.linearTilingFeatures;
        properties3->bufferFeatures = properties2.formatProperties.bufferFeatures;

        // Automatically promote for supported formats.
        //properties3->optimalTilingFeatures =
        //    promote_storage_usage(ext, format, properties3->optimalTilingFeatures);
        //properties3->linearTilingFeatures =
        //    promote_storage_usage(ext, format, properties3->linearTilingFeatures);
    }
}

PerFrameContext& Device_Vulkan::GetCurrentFrame()
{
    return m_frames[m_frame_context_index];
}

bool Device_Vulkan::isFormatSupported(DataFormat format)
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_vulkan_context->physicalDevice, ConvertDataFormat(format), &props);
    return ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) && (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT));
}

DescriptorSetAllocator* Device_Vulkan::RequestDescriptorSetAllocator(const DescriptorSetLayout& layout)
{
    size_t hash = 0;
    util::hash_combine(hash, layout.uniform_buffer_mask);
    util::hash_combine(hash, layout.storage_buffer_mask);
    util::hash_combine(hash, layout.sampled_image_mask);
    util::hash_combine(hash, layout.storage_image_mask);
    util::hash_combine(hash, layout.separate_image_mask);
    util::hash_combine(hash, layout.sampler_mask);
    util::hash_combine(hash, layout.input_attachment_mask);

    LOCK_CACHE();
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

