#include "pch.h"
#include "Rendering/Vulkan/RenderDevice_Vulkan.h"

#
#include <GLFW/glfw3.h>
#include <spirv_reflect.h>

#include "Core/Window.h"
#include "Events/EventManager.h"

/*****************/
/**** HELPERS ****/
/*****************/
constexpr VkFormat _ConvertFormat(DataFormat value)
{
    switch(value)
    {
    case DataFormat::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case DataFormat::D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case DataFormat::D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case DataFormat::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case DataFormat::R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }

}

constexpr VkImageType _ConvertImageType(GPUImageType type)
{
    switch (type) 
    {
    case GPUImageType::TYPE_2D:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_2D_ARRAY:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_CUBE:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_CUBE_ARRAY:
        return VK_IMAGE_TYPE_2D;
    case GPUImageType::TYPE_MAX:
        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

constexpr VkImageViewType _Convert2ImageViewType(GPUImageType type)
{
    switch (type) 
    {
    case GPUImageType::TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case GPUImageType::TYPE_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case GPUImageType::TYPE_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case GPUImageType::TYPE_CUBE_ARRAY:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    case GPUImageType::TYPE_MAX:
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

constexpr VkSampleCountFlagBits _ConvertSampleBits(GPUImageSamples sample)
{
    switch (sample) 
    {
    case GPUImageSamples::SAMPLES_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case GPUImageSamples::SAMPLES_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case GPUImageSamples::SAMPLES_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case GPUImageSamples::SAMPLES_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case GPUImageSamples::SAMPLES_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case GPUImageSamples::SAMPLES_32:
        return VK_SAMPLE_COUNT_32_BIT;
    case GPUImageSamples::SAMPLES_64:
        return VK_SAMPLE_COUNT_64_BIT;
    default:
        return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    }
}

constexpr VkCompareOp _ConvertCompareOp(CompareOperation op)
{
    switch (op) 
    {
    case CompareOperation::NEVER:
        return VK_COMPARE_OP_NEVER;
    case CompareOperation::LESS:
        return VK_COMPARE_OP_LESS;
    case CompareOperation::EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case CompareOperation::LESS_OR_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOperation::GREATER:
        return VK_COMPARE_OP_GREATER;
    case CompareOperation::NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOperation::GREATER_OR_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOperation::ALWAYS:
        return VK_COMPARE_OP_ALWAYS;
    default:
    case CompareOperation::MAX_ENUM:
        return VK_COMPARE_OP_MAX_ENUM;
    }
}
constexpr VkPrimitiveTopology _ConvertPrimitiveTopology(PrimitiveTopology pri)
{
    switch (pri) {
    case PrimitiveTopology::TRANGLE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::LINE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;        
    case PrimitiveTopology::POINT_LIST:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PrimitiveTopology::TRIANGLE_STRIPS_WITH_RESTART_INDEX:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
    default:
        return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }

}

constexpr VkImageLayout _ConvertImageLayout(GPUImageLayout layout)
{
    switch (layout) {
    case GPUImageLayout::UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case GPUImageLayout::GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case GPUImageLayout::COLOR_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case GPUImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case GPUImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case GPUImageLayout::SHADER_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case GPUImageLayout::TRANSFER_SRC_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case GPUImageLayout::TRANSFER_DST_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case GPUImageLayout::PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
}

bool RenderDevice_Vulkan::Init() 
{
    CORE_LOGI("===============Init Vulkan Driver==================")

    recreateSwapchain = false;
    currentFrame = 0;
    // Default frambuffer size
    frameBufferWidth = Window::Instance()->GetFrambufferWidth();
    frameBufferHeight = Window::Instance()->GetFrambufferHeight();

    CreateVulkanInstance();
#ifdef QK_DEBUG_BUILD
    CreateDebugMessenger();
#endif
    CreateVulkanSurface();

    this->device = new VulkanDevice(*this);
    this->swapChain = new VulkanSwapChain(*this, frameBufferWidth, frameBufferHeight);

    InitExtendFunctions();
    CORE_LOGD("Vulkan Extend functions Found")

    CreatePipeLineLayout();
    
    // Create Copy Command
    VkCommandPoolCreateInfo copy_pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    copy_pool_create_info.queueFamilyIndex = device->transferQueueIndex;
    copy_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device->logicalDevice, &copy_pool_create_info, nullptr, &copyCmd.cmdPool))

    VkCommandBufferAllocateInfo copy_buffer_allc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    copy_buffer_allc_info.commandPool = copyCmd.cmdPool;
    copy_buffer_allc_info.commandBufferCount = 1;
    copy_buffer_allc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    copy_buffer_allc_info.pNext = nullptr;
    VK_CHECK(vkAllocateCommandBuffers(device->logicalDevice, &copy_buffer_allc_info, &copyCmd.cmdBuffer))

    VkFenceCreateInfo copy_fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    copy_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    copy_fence_create_info.pNext = nullptr;
    VK_CHECK(vkCreateFence(device->logicalDevice, &copy_fence_create_info, nullptr, &copyCmd.fence))

    CORE_LOGD("Copy command created")

    CreateFrameData();
    CORE_LOGI("Frame data created")

    // Register vulkan driver callback functions
    EventManager::Instance().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event);});
    CORE_LOGD("============Vulkan Driver Initialized==============")

    return true;
}

void RenderDevice_Vulkan::ShutDown() 
{
    CORE_LOGI("Shutdown Vulkan driver...")
    vkDeviceWaitIdle(device->logicalDevice);

    CORE_LOGI("Destroying per-frame data...")
    CORE_LOGD("Destroying command lists...")
    for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; i++) {
        for (size_t j = 0; j < QUEUE_TYPE_MAX_ENUM; j++) {
            for (auto& cmdList : frames[i].queueCommands[j].cmds) {
                // Destroy semaphores
                vkDestroySemaphore(device->logicalDevice, cmdList.cmdCompleteSemaphore, nullptr);
                vkDestroyCommandPool(device->logicalDevice, cmdList.cmdPool, nullptr);
            }
            vkDestroyFence(device->logicalDevice, frames[i].queueCommands[j].inFlghtfence, nullptr);
        }
        vkDestroySemaphore(device->logicalDevice, frames[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device->logicalDevice, frames[i].renderCompleteSemaphore, nullptr);
    }

    CORE_LOGD("Destroying copy commands...")
    vkDestroyFence(device->logicalDevice, copyCmd.fence, nullptr);
    vkDestroyCommandPool(device->logicalDevice, copyCmd.cmdPool, nullptr);

    CORE_LOGD("Destroying Pipeline layout...")
    vkDestroyPipelineLayout(device->logicalDevice, graphicPipeLineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device->logicalDevice, sceneDataDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(device->logicalDevice, materialDescriptorLayout, nullptr);

    CORE_LOGI("Destroying vulkan swapchain...")
    delete swapChain;
    
    CORE_LOGI("Destroying vulkan device...")
    delete device;
    
    CORE_LOGI("Destroying vulkan surface...")
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Destroying debug messenger...")
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    func(vkInstance, debugMessenger, nullptr);
#endif

    CORE_LOGI("Destroying vulkan instance...")
    vkDestroyInstance(vkInstance, nullptr);

    CORE_LOGI("Vulkan driver shutdown successfully.")

}

bool RenderDevice_Vulkan::BeiginFrame(f32 deltaTime) 
{
    // Resize the swapchain if needed. 
    if (recreateSwapchain) {
        CORE_LOGI("Resizing swapchain...")
        swapChain->Resize(frameBufferWidth, frameBufferHeight);
        recreateSwapchain = false;
    }

    auto& frame = frames[currentFrame];

    vkWaitForFences(device->logicalDevice, 1, &frame.queueCommands[QUEUE_TYPE_GRAPHICS].inFlghtfence, true, 1000000000);
	vkResetFences(device->logicalDevice, 1, &frame.queueCommands[QUEUE_TYPE_GRAPHICS].inFlghtfence);

    // Reset queue task status
    for (size_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++)
    {
        frame.queueCommands[i].cmdsCount = 0;
    }

    if(!swapChain->AquireNextImageIndex(frame.imageAvailableSemaphore, VK_NULL_HANDLE, 10000000, &currentSwapChainImageIdx))
    {
        CORE_LOGW("Acquire swapchain image failed. Skipping...")
        return false;
    }


    return true;
}

bool RenderDevice_Vulkan::EndFrame(f32 deltaTime) 
{
    auto& frame = frames[currentFrame];

    // TODO: Support multithreding. Only support grahpics queue for now.
    // Submit queue tasks of this frame
    VkSemaphoreSubmitInfo wait_semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    wait_semaphore_info.pNext = nullptr;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    wait_semaphore_info.semaphore = frame.imageAvailableSemaphore;
    wait_semaphore_info.value = 0;
    VkSemaphoreSubmitInfo signal_semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    signal_semaphore_info.pNext = nullptr;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signal_semaphore_info.semaphore = frame.renderCompleteSemaphore;
    signal_semaphore_info.value = 0;

    std::vector<VkCommandBufferSubmitInfo> submitCmds;
    for (size_t i = 0; i < frame.queueCommands[QUEUE_TYPE_GRAPHICS].cmdsCount; i++) {
        VkCommandBufferSubmitInfo& cmd_submit_info = submitCmds.emplace_back();
        cmd_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_submit_info.commandBuffer = frame.queueCommands[QUEUE_TYPE_GRAPHICS].cmds[i].cmdBuffer;
    }

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
    submit_info.commandBufferInfoCount = submitCmds.size();
    submit_info.pCommandBufferInfos = submitCmds.data();

    extendFunctions.pVkQueueSubmit2KHR(device->graphicQueue, 1, 
        &submit_info, frame.queueCommands[QUEUE_TYPE_GRAPHICS].inFlghtfence);

    // prepare present
    VkSwapchainKHR swapchain = swapChain->handle;
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &frames[currentFrame].renderCompleteSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &currentSwapChainImageIdx;
	VkResult presentResult = vkQueuePresentKHR(device->presentQueue, &presentInfo);

    if (presentResult != VK_SUCCESS && presentResult != VK_ERROR_OUT_OF_DATE_KHR
		&& presentResult != VK_SUBOPTIMAL_KHR) {
        CORE_ASSERT_MSG(false, "failed to present swap chain image!")
    }
    currentFrame = (currentFrame + 1) % MAX_FRAME_NUM_IN_FLIGHT;
    return true;
}

void RenderDevice_Vulkan::OnWindowResize(const WindowResizeEvent &event) 
{
    frameBufferWidth = event.width;
    frameBufferHeight = event.height;
    recreateSwapchain = true;
    CORE_LOGI("Vulkan driver hook resize event. Width: {} Height: {}", frameBufferWidth, frameBufferHeight)
}

void RenderDevice_Vulkan::ImageCreate(const GPUImageDesc &desc, GPUImage *outImage)
{
    CORE_DEBUG_ASSERT(outImage->internal == nullptr)
    
    auto internal_state = new GPUImage_Vulkan();

	VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.format = _ConvertFormat(desc.format);
    create_info.imageType = _ConvertImageType(desc.type);
    create_info.extent.width = desc.width;
    create_info.extent.height = desc.height;
    create_info.extent.depth = desc.depth;
    create_info.mipLevels = desc.mipLevels;
    create_info.arrayLayers = desc.arraySize;
    create_info.samples = _ConvertSampleBits(desc.samples);
    create_info.tiling = (desc.usageBits & GPUIMAGE_USAGE_CPU_READ_BIT)? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

    if (desc.usageBits & GPUIMAGE_USAGE_SAMPLING_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_STORAGE_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_CAN_COPY_FROM_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_CAN_COPY_TO_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (desc.usageBits & GPUIMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        create_info.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    // TODO: Make sharing mode configurable
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Allocate memory

    // TODO: allocate a pool for small allocation
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.flags = (desc.usageBits & GPUIMAGE_USAGE_CPU_READ_BIT)? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0;
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    // Create image
    VK_CHECK(vmaCreateImage(device->vmaAllocator, &create_info, &allocCreateInfo, &internal_state->handle, &internal_state->allocation.handle, &internal_state->allocation.info))

    // Create imageview
    VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_create_info.image = internal_state->handle;
    image_view_create_info.viewType = _Convert2ImageViewType(desc.type);
    image_view_create_info.format = _ConvertFormat(desc.format);
    // TODO: Make components configurable
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.levelCount = create_info.mipLevels;
	image_view_create_info.subresourceRange.layerCount = create_info.arrayLayers;
    if (desc.usageBits & GPUIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VK_CHECK(vkCreateImageView(device->logicalDevice, &image_view_create_info, nullptr, &internal_state->view))

    // Bookkeep
    outImage->desc = desc;
    outImage->internal = internal_state;
}

void RenderDevice_Vulkan::ImageFree(GPUImage* image)
{   
    if (image->desc.isSwapChainImage) {
        CORE_LOGW("You can not destroy swapChain image manually!")
    }
    
    GPUImage_Vulkan* internal_img =  (GPUImage_Vulkan*)image->internal;

    vkDestroyImageView(device->logicalDevice, internal_img->view, nullptr);
    if (internal_img->allocation.handle) {
        vmaDestroyImage(device->vmaAllocator, internal_img->handle, internal_img->allocation.handle);
    }
    image->internal = nullptr;
}

void RenderDevice_Vulkan::ShaderCreateFromBytes(ShaderStage stage, void* byteCode, size_t byteCount, Shader* outShader)
{
    auto internal_data = new Shader_Vulkan();
    outShader->internal = internal_data;
    outShader->stage = stage;

    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = byteCount;
    createInfo.pCode = (const u32*)byteCode;
    VK_CHECK(vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &internal_data->shaderModule))

    internal_data->stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    internal_data->stageInfo.module = internal_data->shaderModule;
    internal_data->stageInfo.pName = "main";
    switch (stage) 
    {
    case ShaderStage::STAGE_COMPUTE:
        internal_data->stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    case ShaderStage::STAGE_VERTEX:
        internal_data->stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderStage::STAGE_FRAGEMNT:
        internal_data->stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    default:
        internal_data->stageInfo.stage = VK_SHADER_STAGE_ALL;
        break;
    }
    

}

bool RenderDevice_Vulkan::ShaderCreateFromSpvFile(ShaderStage stage, const std::string &name, Shader *outShader)
{
    std::string path = "Assets/Shaders/Spirv"+ name;
    // open the file. With cursor at the end
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        CORE_LOGE("Failed to open shader file: {}", path)
        return false;
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint8_t> buffer(fileSize);

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    ShaderCreateFromBytes(stage, buffer.data(), buffer.size(), outShader);

    return true;

}

void RenderDevice_Vulkan::ShaderFree(Shader *shader)
{
    auto internal_data = (Shader_Vulkan*)shader->internal;
    if (internal_data->shaderModule) {
        vkDestroyShaderModule(device->logicalDevice, internal_data->shaderModule, nullptr);
    }
    // TODO: Destroy other stuffs
}

bool RenderDevice_Vulkan::GraphicPipeLineCreate(PipeLineDesc &desc, PipeLine* outPSO)
{
    auto internal_data = new PipeLine_Vulkan();
    outPSO->internal = internal_data;
    outPSO->desc = desc;

    // Vertex
    // TODO: Make this configurable
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    if (!desc.vertexAttribs.empty()) {
        VkVertexInputBindingDescription binding_description;
        binding_description.binding = 0;  // Binding index
        binding_description.stride = sizeof(Vertex3D);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; 

        std::vector<VkVertexInputAttributeDescription> attribs;
        if (!desc.vertexAttribs.empty()) {
            attribs.resize(desc.vertexAttribs.size());
            for (size_t i = 0; i < attribs.size(); ++i) {
                attribs[i].binding = 0;
                attribs[i].format = _ConvertFormat(desc.vertexAttribs[i].format);
                attribs[i].location = desc.vertexAttribs[i].location;
                attribs[i].offset = desc.vertexAttribs[i].offset;
            }
        }
        vertex_input_create_info.vertexBindingDescriptionCount = 1;
        vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
        vertex_input_create_info.vertexAttributeDescriptionCount = attribs.size();
        vertex_input_create_info.pVertexAttributeDescriptions = attribs.data();
    }
    else {
        vertex_input_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_create_info.vertexAttributeDescriptionCount = 0;
    }


    // Input assembly.
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly_create_info.topology = _ConvertPrimitiveTopology(desc.primitiveTopo);
	input_assembly_create_info.primitiveRestartEnable = (desc.primitiveTopo == PrimitiveTopology::TRIANGLE_STRIPS_WITH_RESTART_INDEX);

    // TODO: Support tessellation

    // Viewport.
	VkPipelineViewportStateCreateInfo viewport_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport_state_create_info.viewportCount = 1; 
	viewport_state_create_info.scissorCount = 1;

    // Rasterization.
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterization_state_create_info.depthClampEnable = desc.rasterState.enableDepthClamp;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = desc.rasterState.isWireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
	rasterization_state_create_info.frontFace = (desc.rasterState.frontFaceCCW ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE );
    // TODO: Make depth bias configurable
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0;
	rasterization_state_create_info.depthBiasClamp = 0;
	rasterization_state_create_info.depthBiasSlopeFactor = 0;
	rasterization_state_create_info.lineWidth = desc.rasterState.lineWidth;
    switch (desc.rasterState.cullMode)
    {
    case CullMode::BACK:
        rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        break;
    case CullMode::FRONT:
        rasterization_state_create_info.cullMode = VK_CULL_MODE_FRONT_BIT;
        break;
    case CullMode::NONE:
    default:
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        break;
    }

    // Multisample
    // TODO: Support multisample. Just hard coded for now
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = 0;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    // Depth stencil.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = desc.depthStencilState.enableDepthTest;
    depth_stencil_state_create_info.depthWriteEnable = desc.depthStencilState.enableDepthWrite;
    depth_stencil_state_create_info.depthCompareOp = _ConvertCompareOp(desc.depthStencilState.depthCompareOp);
    // TODO: Support stencil test and depth bounds test
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;

    // Blend state
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // TODO: Support logic operation
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_MAX_ENUM;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // TODO: Support multiple color attachments and other blend factor
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

    // Dynamic state
    // TODO: Make this configurable
    const u32 dynamic_state_count = 3;
    VkDynamicState dynamic_states[dynamic_state_count] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    // Shader stages
    CORE_DEBUG_ASSERT(desc.vertShader != nullptr && desc.fragShader != nullptr)
    //TODO: Make configurable
    auto vert_shader_internal = (Shader_Vulkan*)desc.vertShader;
    auto frag_shader_internal = (Shader_Vulkan*)desc.fragShader;
    VkPipelineShaderStageCreateInfo shader_stage_info[] = { vert_shader_internal->stageInfo, frag_shader_internal->stageInfo};

    // Dynamic rendering
    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    std::vector<VkFormat> vk_color_attachment_format;
    vk_color_attachment_format.resize(desc.colorAttachmentFormats.size());
    for (size_t i = 0; i < desc.colorAttachmentFormats.size(); i++) {
        vk_color_attachment_format[i] = _ConvertFormat(desc.colorAttachmentFormats[i]);
    }
    renderingInfo.colorAttachmentCount = vk_color_attachment_format.size();
    renderingInfo.pColorAttachmentFormats = vk_color_attachment_format.data();
    renderingInfo.depthAttachmentFormat = _ConvertFormat(desc.depthAttachmentFormat);

    // Finally, fill pipeline create info
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.pStages = shader_stage_info;
	pipeline_create_info.pVertexInputState = &vertex_input_create_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pViewportState = &viewport_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
	pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
	pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	pipeline_create_info.layout = graphicPipeLineLayout; //TODO: Make configurable
    pipeline_create_info.pNext = &renderingInfo;
    pipeline_create_info.renderPass = nullptr;

    return true;
}

bool RenderDevice_Vulkan::ComputePipeLineCreate(Shader *comp_shader, PipeLine *outPSO)
{
    auto internal_data = new PipeLine_Vulkan();
    outPSO->internal = internal_data;
    outPSO->desc.compShader = comp_shader;

    return true;
}

CommandList RenderDevice_Vulkan::CommandListBegin(QueueType type)
{
    auto& frame = frames[currentFrame];
    auto& queueTask = frame.queueCommands[type];
    u32 cmd_count = queueTask.cmdsCount++;
    if (cmd_count >= queueTask.cmds.size()) {
        // Create a new Command list is needed
        CommandList_Vulkan newCmdList;
        for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; ++i)
        {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            // TODO: Support other queue types
            switch (type)
            {
            case QUEUE_TYPE_GRAPHICS:
                poolInfo.queueFamilyIndex = device->graphicQueueIndex;
                newCmdList.type = QUEUE_TYPE_GRAPHICS;
                break;
            case QUEUE_TYPE_ASYNC_COMPUTE:
                poolInfo.queueFamilyIndex = device->computeQueueIndex;
                newCmdList.type = QUEUE_TYPE_ASYNC_COMPUTE;
                CORE_ASSERT(0)
                break;
            case QUEUE_TYPE_ASYNC_TRANSFER:
                poolInfo.queueFamilyIndex = device->transferQueueIndex;
                newCmdList.type = QUEUE_TYPE_ASYNC_TRANSFER;
                CORE_ASSERT(0)
                break;
            default:
                CORE_ASSERT_MSG(0, "Queue Type not handled."); // queue type not handled
                break;
            }
            poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            VK_CHECK(vkCreateCommandPool(device->logicalDevice, &poolInfo, nullptr, &newCmdList.cmdPool))
        
            VkCommandBufferAllocateInfo commandBufferInfo = {};
            commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferInfo.commandBufferCount = 1;
            commandBufferInfo.commandPool = newCmdList.cmdPool;
            commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VK_CHECK(vkAllocateCommandBuffers(device->logicalDevice, &commandBufferInfo, &newCmdList.cmdBuffer))

            VkSemaphoreCreateInfo semCreateInfo = {};
			semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VK_CHECK(vkCreateSemaphore(device->logicalDevice, &semCreateInfo, nullptr, &newCmdList.cmdCompleteSemaphore))

            frames[i].queueCommands[type].cmds.push_back(newCmdList);
        }
    }

    CommandList_Vulkan& internal_cmdList = queueTask.cmds[cmd_count];

    // Reset status
    vkResetCommandPool(device->logicalDevice, internal_cmdList.cmdPool, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional
    VK_CHECK(vkBeginCommandBuffer(internal_cmdList.cmdBuffer, &beginInfo))

    internal_cmdList.renderPassBeginBarriers.clear();
    internal_cmdList.renderPassEndBarriers.clear();
    internal_cmdList.waitForCmds.clear();
    internal_cmdList.waitForSwapchainImage = false;

    CommandList cmd;
    cmd.internal = &internal_cmdList;

    return cmd;
}

void RenderDevice_Vulkan::CommandListEnd(CommandList cmd)
{
    vkEndCommandBuffer(((CommandList_Vulkan*)cmd.internal)->cmdBuffer);
}

void RenderDevice_Vulkan::PipeLineFree(PipeLine *pipeline)
{
    auto internal_data = (PipeLine_Vulkan*)pipeline->internal;
    vkDestroyPipeline(device->logicalDevice, internal_data->pipeline, nullptr);
}

void RenderDevice_Vulkan::CmdBeginRenderPass(CommandList cmd, RenderPass &renderPass)
{
    CORE_DEBUG_ASSERT(renderPass.numColorAttachments < RenderPass::MAX_COLOR_ATTHACHEMNT_NUM)
    CORE_DEBUG_ASSERT(renderPass.numResolveAttachments < RenderPass::MAX_COLOR_ATTHACHEMNT_NUM)

    auto& internal_cmd_list = GetInternalCommanddList(cmd);
    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.layerCount = 1;
    rendering_info.renderArea.offset.x = 0;
    rendering_info.renderArea.offset.y = 0;
    rendering_info.renderArea.extent.width = 0;
    rendering_info.renderArea.extent.height = 0;
    rendering_info.colorAttachmentCount = renderPass.numColorAttachments;

    VkRenderingAttachmentInfo color_attachments[RenderPass::MAX_COLOR_ATTHACHEMNT_NUM] = {};
    VkRenderingAttachmentInfo depth_attachment = {};

    auto convertLoadOp = [](RenderPass::AttachmentLoadOp op) -> VkAttachmentLoadOp {
        switch (op) {
        default:
        case RenderPass::AttachmentLoadOp::CLEAR:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case RenderPass::AttachmentLoadOp::LOAD:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RenderPass::AttachmentLoadOp::DONTCARE:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
    };

    auto convertStoreOp = [](RenderPass::AttachmentStoreOp op) ->VkAttachmentStoreOp {
        switch (op) {
        default:
        case RenderPass::AttachmentStoreOp::STORE:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case RenderPass::AttachmentStoreOp::DONTCARE:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
    };

    // Color Attachments
    for (size_t i = 0; i < renderPass.numColorAttachments; ++i) {
        const GPUImage* image = renderPass.colorAttachments[i];
        const GPUImageDesc& image_desc = image->desc;
        auto& internal_image = GetInternal(image);

        rendering_info.renderArea.extent.width = std::max(rendering_info.renderArea.extent.width, image_desc.width);
		rendering_info.renderArea.extent.height = std::max(rendering_info.renderArea.extent.height, image_desc.height);

        color_attachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachments[i].imageView = internal_image.view;
        color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].loadOp = convertLoadOp(renderPass.colorAttatchemtsLoadOp[i]);
        color_attachments[i].storeOp = convertStoreOp(renderPass.colorAttatchemtsStoreOp[i]);
        color_attachments[i].clearValue.color.float32[0] = renderPass.clearColors[i].color[0];
        color_attachments[i].clearValue.color.float32[1] = renderPass.clearColors[i].color[1];
        color_attachments[i].clearValue.color.float32[2] = renderPass.clearColors[i].color[2];
        color_attachments[i].clearValue.color.float32[3] = renderPass.clearColors[i].color[3];

        // internal swapchain image state tracking
        if (image_desc.isSwapChainImage) {
            internal_cmd_list.waitForSwapchainImage = true;
        }
    }

    // Resolve attatchments
    for (size_t i = 0; i < renderPass.numResolveAttachments; ++i) {
        color_attachments[i].resolveImageView = GetInternal(renderPass.resolveAttatchments[i]).view;
        color_attachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    // Depth attatchment
    if (renderPass.depthAttatchment != nullptr) {
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = GetInternal(renderPass.depthAttatchment).view;
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = convertLoadOp(renderPass.depthAttachmentLoadOp);
        depth_attachment.storeOp = convertStoreOp(renderPass.depthAttachmentStoreOp);
        depth_attachment.clearValue.depthStencil.depth = renderPass.ClearDepthStencil.depth_stencil.depth;

    }

    rendering_info.pColorAttachments = renderPass.numColorAttachments > 0? color_attachments : nullptr;
    rendering_info.pDepthAttachment = renderPass.depthAttatchment ? &depth_attachment : nullptr;
    //TODO: Support stencil test
    rendering_info.pStencilAttachment = nullptr;
    rendering_info.pNext = nullptr;

    extendFunctions.pVkCmdBeginRenderingKHR(internal_cmd_list.cmdBuffer, &rendering_info);
}

void RenderDevice_Vulkan::CmdEndRenderPass(CommandList cmd)
{
    auto internal_cmdList = GetInternalCommanddList(cmd);

    extendFunctions.pVkCmdEndRenderingKHR(internal_cmdList.cmdBuffer);

    if (!internal_cmdList.renderPassEndBarriers.empty()) {
        VkDependencyInfo dependency_info = {};
        dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(internal_cmdList.renderPassEndBarriers.size());
        dependency_info.pImageMemoryBarriers = internal_cmdList.renderPassEndBarriers.data();

        extendFunctions.pVkCmdPipelineBarrier2KHR(internal_cmdList.cmdBuffer, &dependency_info);
        internal_cmdList.renderPassEndBarriers.clear();
    }

}

void RenderDevice_Vulkan::CmdPipeLineBarriers(CommandList cmd, PipelineMemoryBarrier *memoryBarriers, u32 memoryBarriersCount, PipelineImageBarrier *iamgeBarriers, u32 iamgeBarriersCount, PipelineBufferBarrier *bufferBarriers, u32 bufferBarriersCount)
{
    CORE_DEBUG_ASSERT(bufferBarriers == nullptr);   // do not support buffer barrier for now

    auto& internal_cmd_list = GetInternal(cmd);
    auto& memory_barriers =  internal_cmd_list.memoryBarriers;
    auto& image_barriers = internal_cmd_list.imageBarriers;
    auto& buffer_barriers = internal_cmd_list.bufferBarriers;
    CORE_DEBUG_ASSERT(memory_barriers.empty() && image_barriers.empty() && buffer_barriers.empty())

    // Memory Barriers
    for (size_t i = 0; i < memoryBarriersCount; ++i) {
        PipelineMemoryBarrier& memory_barrier = memoryBarriers[i];
        VkMemoryBarrier2 vk_memory_barrier = {};
        vk_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		vk_memory_barrier.pNext = nullptr;
        vk_memory_barrier.srcStageMask = (VkPipelineStageFlagBits2)memory_barrier.srcStageBits;
        vk_memory_barrier.srcAccessMask = (VkAccessFlagBits2)memory_barrier.srcMemoryAccessBits;
        vk_memory_barrier.dstStageMask = (VkPipelineStageFlagBits2)memory_barrier.dstStageBits;
        vk_memory_barrier.dstAccessMask = (VkAccessFlagBits2)memory_barrier.dstMemoryAccessBits;
        memory_barriers.push_back(vk_memory_barrier);
    }

    // Image Barriers
    for (size_t i = 0; i < iamgeBarriersCount; ++i) {
        PipelineImageBarrier& image_barrier = iamgeBarriers[i];
        auto& image_desc = image_barrier.image->desc;
        auto image_format = image_desc.format;
        VkImageMemoryBarrier2 vk_image_barrier = {};
        vk_image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		vk_image_barrier.pNext = nullptr;
        vk_image_barrier.srcStageMask = (VkPipelineStageFlagBits2)image_barrier.srcStageBits;
        vk_image_barrier.srcAccessMask = (VkAccessFlagBits2)image_barrier.srcMemoryAccessBits;
        vk_image_barrier.dstStageMask = (VkPipelineStageFlagBits2)image_barrier.dstStageBits;
        vk_image_barrier.dstAccessMask = (VkAccessFlagBits2)image_barrier.dstMemoryAccessBits;
        vk_image_barrier.image = GetInternal(image_barrier.image).handle;
        vk_image_barrier.oldLayout = _ConvertImageLayout(image_barrier.layoutBefore);
        vk_image_barrier.newLayout = _ConvertImageLayout(image_barrier.layoutAfter);
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

        image_barriers.push_back(vk_image_barrier);
    }
    
    // TODO:Support buffer barrier

    if (!memory_barriers.empty() || !image_barriers.empty() || !buffer_barriers.empty()) {
        VkDependencyInfo dependency_info = {};
        dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.memoryBarrierCount = static_cast<uint32_t>(memory_barriers.size());
        dependency_info.pMemoryBarriers = memory_barriers.data();
        dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size());
        dependency_info.pBufferMemoryBarriers = buffer_barriers.data();
        dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size());
        dependency_info.pImageMemoryBarriers = image_barriers.data();

        extendFunctions.pVkCmdPipelineBarrier2KHR(internal_cmd_list.cmdBuffer, &dependency_info);

        memory_barriers.clear();
        image_barriers.clear();
        buffer_barriers.clear();
    }
}

void RenderDevice_Vulkan::CreateVulkanInstance() {
    CORE_LOGI("Creating vulkan instance...")
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Quark Engine Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Quark Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // Add flag for drivers that support portability subset
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Get extensions for window support
    auto extensions = GetRequiredExtensions();

    // For debug purpuse
#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Required vulkan instance extensions:")
    for(const auto& s : extensions)
        CORE_LOGD("  {}", s);
#endif

    CORE_LOGD("Checking vulkan instance extensions support...")
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for (u32 i = 0; i < extensions.size(); ++i) {
        bool found = false;
        for (u32 j = 0; j < availableExtensions.size(); ++j) {
            if (strcmp(extensions[i], availableExtensions[j].extensionName)==0) {
                found = true;
                break;
            }
        }

        if (!found) {
            CORE_LOGC("  Required extension not found: {}", extensions[i]);
        }
    }
    
    // All extensions are supported
    CORE_LOGD("All required vulkan instance extensions are supported.")
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers?
#ifdef QK_DEBUG_BUILD
    CORE_LOGD("Validation layers enabled. Checking...")

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // checking
    for (auto layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            CORE_LOGC("  Required extension not found: {}", layerName);
        }
            
    }

    CORE_LOGD("All required vulkan validation layers are supported.")
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vkInstance))
    CORE_LOGI("Vulkan instance created")

}

void RenderDevice_Vulkan::CreateDebugMessenger()
{
    CORE_LOGD("Createing vulkan debug messenger...")
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VkDebugCallback;

#ifdef QK_DEBUG_BUILD
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    CORE_ASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(vkInstance, &createInfo, nullptr, &debugMessenger))
#endif

    CORE_LOGD("Vulkan debug messenger created")
}

void RenderDevice_Vulkan::CreateVulkanSurface()
{
    // surface的具体创建过程是要区分平台的，这里直接用GLFW封装好的接口来创建
    VK_CHECK(glfwCreateWindowSurface(vkInstance, (GLFWwindow*)Window::Instance()->GetNativeWindow(), nullptr, &vkSurface))
    CORE_LOGI("Vulkan surface created")
}

void RenderDevice_Vulkan::CreatePipeLineLayout()
{
    // Scene data descriptor set
    VkDescriptorSetLayoutBinding scene_descriptor_set_binding;
    scene_descriptor_set_binding.binding = 0;
    scene_descriptor_set_binding.descriptorCount = 1;
    scene_descriptor_set_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    scene_descriptor_set_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo scene_set_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    scene_set_create_info.bindingCount = 1;
    scene_set_create_info.pBindings = &scene_descriptor_set_binding;
    scene_set_create_info.flags = 0;
    scene_set_create_info.pNext = nullptr;
    VK_CHECK(vkCreateDescriptorSetLayout(device->logicalDevice, &scene_set_create_info, nullptr, &sceneDataDescriptorLayout))

    // Material data descriptor set
    std::vector<VkDescriptorSetLayoutBinding> material_descriptor_set_bindings(3);
    material_descriptor_set_bindings[0].binding = 0;
    material_descriptor_set_bindings[0].descriptorCount = 1;
    material_descriptor_set_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    material_descriptor_set_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    material_descriptor_set_bindings[1].binding = 1;
    material_descriptor_set_bindings[1].descriptorCount = 1;
    material_descriptor_set_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    material_descriptor_set_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    material_descriptor_set_bindings[2].binding = 2;
    material_descriptor_set_bindings[2].descriptorCount = 1;
    material_descriptor_set_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    material_descriptor_set_bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo material_set_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    material_set_create_info.bindingCount = material_descriptor_set_bindings.size();
    material_set_create_info.pBindings = material_descriptor_set_bindings.data();
    material_set_create_info.flags = 0;
    material_set_create_info.pNext = nullptr;
    VK_CHECK(vkCreateDescriptorSetLayout(device->logicalDevice, &material_set_create_info, nullptr, &materialDescriptorLayout))

    // Push constants
    VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GpuDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Finally, pipleline layout
	VkDescriptorSetLayout layouts[] = { sceneDataDescriptorLayout, materialDescriptorLayout };

	VkPipelineLayoutCreateInfo graphic_pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	graphic_pipeline_layout_create_info.setLayoutCount = 2;
	graphic_pipeline_layout_create_info.pSetLayouts = layouts;
    graphic_pipeline_layout_create_info.pushConstantRangeCount = 1;
	graphic_pipeline_layout_create_info.pPushConstantRanges = &matrixRange;

	VK_CHECK(vkCreatePipelineLayout(device->logicalDevice, &graphic_pipeline_layout_create_info, nullptr, &graphicPipeLineLayout))
    CORE_LOGD("Grahpic pipeline layout created")
}

GPUImage* RenderDevice_Vulkan::GetSwapChainImage()
{
    return &(swapChain->swapChainImages[currentSwapChainImageIdx]);
}

VKAPI_ATTR VkBool32 VKAPI_CALL RenderDevice_Vulkan::VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, 
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
    void* pUserData)
{
    switch (messageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            CORE_LOGE(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            CORE_LOGW(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            CORE_LOGI(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            CORE_LOGT(pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

std::vector<const char*> RenderDevice_Vulkan::GetRequiredExtensions() const
{
    //TODO: make this as an interface of Window class
    // Get extensions for glfw
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions;

    for (unsigned int i = 0; i < glfwExtensionCount; i++)
        extensions.push_back(glfwExtensions[i]);

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif

#ifdef QK_DEBUG_BUILD
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    return extensions;  
}

void RenderDevice_Vulkan::InitExtendFunctions()
{
    extendFunctions.pVkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR)vkGetDeviceProcAddr(
        device->logicalDevice, "vkQueueSubmit2KHR");
    extendFunctions.pVkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(
        device->logicalDevice, "vkCmdBlitImage2KHR");
    extendFunctions.pVkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(
        device->logicalDevice, "vkCmdPipelineBarrier2KHR");
    extendFunctions.pVkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(
        device->logicalDevice, "vkCmdBeginRenderingKHR");
    extendFunctions.pVkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(
        device->logicalDevice, "vkCmdEndRenderingKHR");
}

void RenderDevice_Vulkan::CreateFrameData()
{
    for (size_t i = 0; i < MAX_FRAME_NUM_IN_FLIGHT; i++) {
        for (size_t j = 0; j < QUEUE_TYPE_MAX_ENUM; j++) {
            // Create a fence per queue
            VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            fence_create_info.pNext = nullptr;
            vkCreateFence(device->logicalDevice, &fence_create_info, nullptr, &frames[i].queueCommands[j].inFlghtfence);
        }

        // Create semaphore
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        semaphore_create_info.pNext = nullptr;
        vkCreateSemaphore(device->logicalDevice, &semaphore_create_info, nullptr, &frames[i].imageAvailableSemaphore);
        vkCreateSemaphore(device->logicalDevice, &semaphore_create_info, nullptr, &frames[i].renderCompleteSemaphore);
    }
}