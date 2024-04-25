#include "Graphics/Vulkan/RendererVulkan.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Application/Application.h"
#include "Graphics/Vulkan/CreateInfoUtils.h"
#include "Graphics/Vulkan/MaterialVulkan.h"
#include "Graphics/Vulkan/VulkanExtentionFuncs.h"
#include "Graphics/Vulkan//VulkanImageCommands.h"
#include "Events/EventManager.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_dynamic_rendering",
    "VK_KHR_synchronization2",
    "VK_KHR_copy_commands2"
#ifdef __APPLE__
    ,"VK_KHR_portability_subset"
#endif
};

RendererVulkan* RendererVulkan::instance_ = nullptr;

void RendererVulkan::Creat()
{
    RendererVulkan::instance_ = new RendererVulkan();
}

// ----------------------------------------Public Standard API----------------------------------------------
void RendererVulkan::Initialize()
{
    // get application's window
    window_ = Application::Instance().GetWindow().GetNativeWindow();
    int width = 0, height = 0;
    glfwGetFramebufferSize(Application::Instance().GetWindow().GetNativeWindow(), &width, &height);
    windowExtent_.width = width;
    windowExtent_.height = height;

    // init vulkan objects
    CreateVkInstance();
    CreateSurface();
    PickGPU();
    CreateLogicalDevice();
    CreateMemoryAllocator();
    CreateSwapChain();
    CreateFrameBuffers();
    CreateCommands();
    CreateSyncObjects();
    CreateBuiltInDescriptorAllocators();
    CreateBackGroundPipeLines();
    CreateDefaultResources();
    InitImgui();

    metalRoughMaterial.BuildPipelines();
    mainDeletionQueue_.push_deletor([this]() {
        metalRoughMaterial.ClearResources();
    });

    currentFrame_ = 0;
    currentBackgroundEffect_ = 0;

    // add event drawback
    EventManager::Instance().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) {OnWindowResize(event);});

    XE_CORE_INFO("Init Vulkan Render Device")

}

void RendererVulkan::Finalize()
{
    vkDeviceWaitIdle(vkDevice_);

    mainDeletionQueue_.flush();

    // ------------------------------Hard Engine Structure-----------------------------

    // destroy immediate submit structures
    vkDestroyFence(vkDevice_, immFence_, nullptr);
    vkDestroyCommandPool(vkDevice_, immCommandPool_, nullptr);

    // destroy frame data
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        vkDestroyCommandPool(vkDevice_, frameData_[i].commandPool, nullptr);

        //destroy sync objects
        vkDestroyFence(vkDevice_, frameData_[i].renderFence, nullptr);
        vkDestroySemaphore(vkDevice_, frameData_[i].renderSemaphore, nullptr);
        vkDestroySemaphore(vkDevice_, frameData_[i].swapchainSemaphore, nullptr);
        frameData_[i].frameDescriptorPool.DestroyPools();
        frameData_[i].deletionQueue.flush();
    }

    DestroySwapChain();
    vmaDestroyAllocator(vmaAllocator_);
    vkDestroyDevice(vkDevice_, nullptr);
    vkDestroySurfaceKHR(vkInstance_, surface_, nullptr);
    if (enableValidationLayers) {
        vkutil::DestroyDebugUtilsMessengerEXT(vkInstance_, debugMessenger_, nullptr);
    }
    vkDestroyInstance(vkInstance_, nullptr);
}

void RendererVulkan::BeginFrame()
{
    // wait until the gpu has finished rendering the last frame.
    vkWaitForFences(vkDevice_, 1, &frameData_[currentFrame_].renderFence, true, 1000000000);
	vkResetFences(vkDevice_, 1, &frameData_[currentFrame_].renderFence);

    // request image from the swapchain
	VkResult e = vkAcquireNextImageKHR(vkDevice_, swapChain_, 1000000000, frameData_[currentFrame_].swapchainSemaphore, nullptr, &currentPresentImage_);
    if (e == VK_ERROR_OUT_OF_DATE_KHR)
        ResizeSwapchain();
    else if (e != VK_SUCCESS && e != VK_SUBOPTIMAL_KHR)
        XE_CORE_ERROR("failed to acquire swap chain image!");

    // refresh frame data
    frameData_[currentFrame_].deletionQueue.flush();
    frameData_[currentFrame_].frameDescriptorPool.ClearPools();

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBuffer cmd = frameData_[currentFrame_].mainCommandBuffer;
	vkResetCommandBuffer(cmd, 0);
	VkCommandBufferBeginInfo cmdBeginInfo = vkutil::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(cmd, &cmdBeginInfo);

}

void RendererVulkan::EndFrame()
{
    VkCommandBuffer cmd = frameData_[currentFrame_].mainCommandBuffer;
    
    vkEndCommandBuffer(cmd);

    // prepare the submission to the queue. 
	VkCommandBufferSubmitInfo cmdinfo = vkutil::command_buffer_submit_info(cmd);	
	VkSemaphoreSubmitInfo waitInfo = vkutil::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frameData_[currentFrame_].swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkutil::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frameData_[currentFrame_].renderSemaphore);	
	VkSubmitInfo2 submit = vkutil::submit_info(&cmdinfo,&signalInfo,&waitInfo);	
	VulkanExtentionFuncs::pVkQueueSubmit2KHR(graphicsQueue_, 1, &submit, frameData_[currentFrame_].renderFence);
 
    // prepare present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapChain_;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &frameData_[currentFrame_].renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &currentPresentImage_;
	VkResult presentResult = vkQueuePresentKHR(graphicsQueue_, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        ResizeSwapchain();
    }
    else if (presentResult != VK_SUCCESS){
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame_  = (currentFrame_ + 1) % FRAME_OVERLAP;
}

void RendererVulkan::TransitionImageLayout(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    PerFrameData currentFrame = frameData_[currentFrame_];
    vkutil::TransitionImage(vkDevice_, currentFrame.mainCommandBuffer, image, currentLayout, newLayout);
}

void RendererVulkan::CopyImagetoImage(VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
    PerFrameData currentFrame = frameData_[currentFrame_];
    vkutil::CopyImageToImage(vkDevice_, currentFrame.mainCommandBuffer, source, destination, srcSize, dstSize);
}

void RendererVulkan::DrawBackGround(VkImageView targetView, VkExtent2D extent)
{
    PerFrameData& currentFrame = frameData_[currentFrame_];
    
    VkDescriptorSet drawImageDescriptor = currentFrame.frameDescriptorPool.Allocate(drawImageDescriptorLayout_);

    DescriptorWriter writer;
	writer.WriteImage(0, targetView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(drawImageDescriptor);

    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
    // select curent compute pipeline
	ComputeEffect& effect = backgroundEffects_[currentBackgroundEffect_];
	// bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.computePipeline.pipeline);
	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,effect.computePipeline.layout, 0, 1, &drawImageDescriptor, 0, nullptr);
	// bind push constants 
	vkCmdPushConstants(cmd, effect.computePipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
	// execute the compute  pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);
}

void RendererVulkan::DrawGeometry(VkImageView colorTargetView, VkImageView depthTargetView, VkExtent2D extent, const DrawContext &context)
{   

    PerFrameData& currentFrame = frameData_[currentFrame_];
    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
    // begin rendering
	VkRenderingAttachmentInfo colorAttachment = vkutil::attachment_info(colorTargetView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = vkutil::depth_attachment_info(depthTargetView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo renderInfo = vkutil::rendering_info(extent, &colorAttachment, &depthAttachment);
	VulkanExtentionFuncs::pVkCmdBeginRenderingKHR(cmd, &renderInfo);

    // allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = CreateUniformBuffer(sizeof(GpuSceneData));
	currentFrame.deletionQueue.push_deletor([=, this]() {
		DestroyGpuBuffer(gpuSceneDataBuffer);
	});

    GpuSceneData* sceneUniformData = (GpuSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
	*sceneUniformData = context.sceneData;

    //create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = currentFrame.frameDescriptorPool.Allocate(gpuSceneDataDescriptorLayout_);
	DescriptorWriter writer;
	writer.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GpuSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.UpdateSet(globalDescriptor);

    GpuPipeLineVulkan* lastPipeline = nullptr;
	GpuMaterialInstance* lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
    auto draw = [&](const RenderObject& r) {
		if (r.material != lastMaterial) {
			lastMaterial = r.material;
			//rebind pipeline and descriptors if the material changed
			if (r.material->pipeline != lastPipeline) {

				lastPipeline = r.material->pipeline;
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,r.material->pipeline->layout, 0, 1,
					&globalDescriptor, 0, nullptr);

				VkViewport viewport = {};
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (float)extent.width;
				viewport.height = (float)extent.height;
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;

				vkCmdSetViewport(cmd, 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = extent.width;
				scissor.extent.height = extent.height;

				vkCmdSetScissor(cmd, 0, 1, &scissor);
			}

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
				&r.material->materialSet, 0, nullptr);
		}
		//rebind index buffer if needed
		if (r.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = r.indexBuffer;
			vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
		// calculate final mesh matrix
		GpuDrawPushConstants push_constants;
		push_constants.worldMatrix = r.transform;
		push_constants.vertexBuffer = r.vertexBufferAddress;
		vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GpuDrawPushConstants), &push_constants);

		vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);

	};

    // draw objects
    for (auto object : context.OpaqueObjects) {
        draw(object);
    }

    // end rendering
    VulkanExtentionFuncs::pVkCmdEndRenderingKHR(cmd);
}

void RendererVulkan::DrawImgui(VkImageView targetView, VkExtent2D extent)
{
    // determine draw extent

    VkCommandBuffer cmd = frameData_[currentFrame_].mainCommandBuffer;
	VkRenderingAttachmentInfo colorAttachment = vkutil::attachment_info(targetView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingInfo renderInfo = vkutil::rendering_info(extent, &colorAttachment, nullptr);

	VulkanExtentionFuncs::pVkCmdBeginRenderingKHR(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	VulkanExtentionFuncs::pVkCmdEndRenderingKHR(cmd);
}

ColorAttachment RendererVulkan::CreateColorAttachment(uint32_t width, uint32_t height, VkFormat format)
{
	VkExtent3D frameBufferExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

    VkImageUsageFlags BufferUsages{};
    BufferUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    BufferUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    BufferUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    BufferUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;   
    return CreateVulkanImage(frameBufferExtent, format, BufferUsages);
}

DepthAttachment RendererVulkan::CreateDepthAttachment(uint32_t width, uint32_t height, VkFormat format)
{
	VkExtent3D frameBufferExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

    VkImageUsageFlags BufferUsages{};
    BufferUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return CreateVulkanImage(frameBufferExtent, format, BufferUsages);
}

UniformBuffer RendererVulkan::CreateUniformBuffer(size_t size)
{
    UniformBuffer newbuffer = CreateVulkanBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    return newbuffer;
}

VertexBuffer RendererVulkan::CreateVertexBuffer(std::span<Vertex> vertices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);


    // create vertex buffer
    VertexBuffer newbuffer = CreateVulkanBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    VMA_MEMORY_USAGE_GPU_ONLY);

    // create buffer for copying
	AllocatedBuffer staging = CreateVulkanBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void* data = staging.allocation->GetMappedData();

    // copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);

    ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;
		vkCmdCopyBuffer(cmd, staging.buffer, newbuffer.buffer, 1, &vertexCopy);
	});

	DestroyGpuBuffer(staging);

    return newbuffer;
}

IndexBuffer RendererVulkan::CreateIndexBuffer(std::span<uint32_t> indices)
{
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	// create index buffer
	AllocatedBuffer newbuffer = CreateVulkanBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

    // create buffer for copying
	AllocatedBuffer staging = CreateVulkanBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void* data = staging.allocation->GetMappedData();

    // copy vertex buffer
	memcpy(data, indices.data(), indexBufferSize);

    ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = 0;
		indexCopy.size = indexBufferSize;
		vkCmdCopyBuffer(cmd, staging.buffer, newbuffer.buffer, 1, &indexCopy);
	});

	DestroyGpuBuffer(staging);

    return newbuffer;
    
}

Texture RendererVulkan::CreateTexture(void* data, uint32_t width, uint32_t height, bool mipmapped)
{
	size_t data_size = width* height * 4;
	AllocatedBuffer staging = CreateVulkanBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	
    memcpy(staging.info.pMappedData, data, data_size);

    VkExtent3D size;
    size.width = width;
    size.height = height;
    size.depth = 1;

	AllocatedImage new_image = CreateVulkanImage(size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    ImmediateSubmit([&](VkCommandBuffer cmd) {
    vkutil::TransitionImage(vkDevice_, cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = size;

    // copy the buffer into the image
    vkCmdCopyBufferToImage(cmd, staging.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
        &copyRegion);

    vkutil::TransitionImage(vkDevice_, cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    DestroyGpuBuffer(staging);

    return new_image;
}

GpuMeshBuffers RendererVulkan::CreateMeshBuffers(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GpuMeshBuffers newSurface;

	// create vertex buffer
	newSurface.vertexBuffer = CreateVulkanBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(vkDevice_, &deviceAdressInfo);

	// create index buffer
	newSurface.indexBuffer = CreateVulkanBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// create buffer for copying
	AllocatedBuffer staging = CreateVulkanBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;
		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;
		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	DestroyGpuBuffer(staging);

	return newSurface;
}
VkShaderModule RendererVulkan::LoadShader(const char *filePath)
{
    // open the file. With cursor at the end
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        XE_CORE_ERROR("RendererVulkan::LoadShader : Failed Open file {}", filePath)
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char*)buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vkDevice_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        XE_CORE_ERROR("Failed create shader module")
    }
    return shaderModule;
}

void RendererVulkan::DestroyGpuImage(const AllocatedImage& image)
{
    vkDestroyImageView(vkDevice_, image.imageView, nullptr);
    vmaDestroyImage(vmaAllocator_, image.image, image.allocation);
}

void RendererVulkan::DestroyGpuBuffer(const AllocatedBuffer &buffer)
{
    vmaDestroyBuffer(vmaAllocator_, buffer.buffer, buffer.allocation);
}

AllocatedImage RendererVulkan::CreateVulkanImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = vkutil::image_create_info(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	if (vmaCreateImage(vmaAllocator_, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr)) {
        XE_CORE_ERROR("RendererVulkan::CreateVulkanImage : Failed to create vulkan image");
    }

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkutil::imageview_create_info(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	if (vkCreateImageView(vkDevice_, &view_info, nullptr, &newImage.imageView)) {
        XE_CORE_ERROR("RendererVulkan::CreateVulkanImage : Failed to create VkImageView")
    }
	return newImage;
}

AllocatedBuffer RendererVulkan::CreateVulkanBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer newBuffer;
	// allocate the buffer
	if (vmaCreateBuffer(vmaAllocator_, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
		&newBuffer.info) != VK_SUCCESS)
        XE_CORE_ERROR("RendererVulkan::CreateVulkanBuffer : Failed to create vulkan buffer");

	return newBuffer;
}

void RendererVulkan::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    vkResetFences(vkDevice_, 1, &immFence_);
	vkResetCommandBuffer(immCommandBuffer_, 0);

	VkCommandBuffer cmd = immCommandBuffer_;
	VkCommandBufferBeginInfo cmdBeginInfo = vkutil::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(cmd, &cmdBeginInfo);
	function(cmd);
	vkEndCommandBuffer(cmd);

	// submit command buffer to the queue and execute it.
	// immFence_ will now block until the graphic commands finish execution
	VkCommandBufferSubmitInfo cmdinfo = vkutil::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkutil::submit_info(&cmdinfo, nullptr, nullptr);
    auto pVkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR)vkGetDeviceProcAddr(vkDevice_, "vkQueueSubmit2KHR");
	pVkQueueSubmit2KHR(graphicsQueue_, 1, &submit, immFence_);

	vkWaitForFences(vkDevice_, 1, &immFence_, true, 9999999999);
}

void RendererVulkan::ResizeSwapchain()
{
	vkDeviceWaitIdle(vkDevice_);
    int width = 0, height = 0;
    glfwGetFramebufferSize(Application::Instance().GetWindow().GetNativeWindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(Application::Instance().GetWindow().GetNativeWindow(), &width, &height);
        glfwWaitEvents();
    }
	DestroySwapChain();

	CreateSwapChain();
}
// --------------------------------------------Create Vulkan Objects---------------------------------------------------
void RendererVulkan::CreateVkInstance()
{
    if (enableValidationLayers && !vkutil::checkValidationLayerSupport(validationLayers)) {
        XE_CORE_ERROR("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "No Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "XEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // add flag for drivers that support portability subset
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // get extensions for window support
    auto extensions = GetRequiredExtensions();
    if (!vkutil::checkInstanceExtensionSupport(extensions))
        XE_CORE_ERROR("Vkinstance extensions are not supported")

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &vkInstance_) != VK_SUCCESS)
        XE_CORE_ERROR("failed to create instance!");
    
    // vkdebugmessenger is just a wrapper of debug callback function
    if (enableValidationLayers)
        CreateDebugMessenger();
}

void RendererVulkan::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = vkutil::VkDebugCallback;

    if (vkutil::CreateDebugUtilsMessengerEXT(vkInstance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS)
        XE_CORE_ERROR("failed to set up debug messenger!");
}

void RendererVulkan::CreateSurface() 
{
    // surface的具体创建过程是要区分平台的，这里直接用GLFW封装好的接口来创建
    if (glfwCreateWindowSurface(vkInstance_, ::Application::Instance().GetWindow().GetNativeWindow(), nullptr, &surface_) != VK_SUCCESS)
        XE_CORE_ERROR("failed to create window surface!");
}

void RendererVulkan::PickGPU()
{       
    // get all GPUs that support vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        XE_CORE_ERROR("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isPhysicalDeviceSuitable(device)) {
            chosenGPU_ = device;
            break;
        }
    }

    if (chosenGPU_ == VK_NULL_HANDLE)
        XE_CORE_ERROR("failed to find a suitable GPU!");
}

void RendererVulkan::CreateSwapChain()
{
    // query swapchain support details
    vkutil::SwapChainSupportDetails swapChainSupport = vkutil::GetSwapChainSupportDetails(chosenGPU_, surface_);

    // choose suitable format, presentmode and resolution
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    // one more image for tripple buffering
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // if maxImageCount = 0, there is no number limit.
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    // populate swapchain create infomation
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.presentMode = presentMode;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT 
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    // how to handle images in different queues
    uint32_t tmpQueueFamilyIndices[] = { queueFamilyIndices_.graphics, queueFamilyIndices_.present };
    if (queueFamilyIndices_.graphics != queueFamilyIndices_.present)
    {
        // share images in graphic and present queue
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = tmpQueueFamilyIndices;
    }
    else
    {   // concurret mode need at least two queues
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // optional
        createInfo.pQueueFamilyIndices = nullptr; // optional
    }

    // no pretransform
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // no need to interacte with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_TRUE;

    // no need to use old swapchain to create a new swapchain
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // create swapchain
    if (vkCreateSwapchainKHR(vkDevice_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
        XE_CORE_ERROR("failed to create swap chain!");
    }

    // get exact number of created images
    vkGetSwapchainImagesKHR(vkDevice_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    // store vkimages
    vkGetSwapchainImagesKHR(vkDevice_, swapChain_, &imageCount, swapChainImages_.data());
    // store format and extent
    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;

    // create swapchain imageviews
    swapchainImageViews_.resize(swapChainImages_.size());
    for (int i = 0 ; i < swapchainImageViews_.size(); i++) {
        VkImageViewCreateInfo view_info = vkutil::imageview_create_info(swapChainImageFormat_, swapChainImages_[i], VK_IMAGE_ASPECT_COLOR_BIT);
        if (vkCreateImageView(vkDevice_, &view_info, nullptr, &swapchainImageViews_[i]) != VK_SUCCESS)
            XE_CORE_ERROR("failed to create image views!");
    }
}

void RendererVulkan::CreateLogicalDevice()
{
    // get queue famliy indices
    queueFamilyIndices_ = vkutil::GetQueueFamilyIndices(chosenGPU_, surface_);

    // required queues for logical device
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices_.graphics, queueFamilyIndices_.present };
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        // assign priorities(0.0 -1.0) to queues to influence the scheduling of command buffer executio
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // required vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = true;
	features13.synchronization2 = true;
	// required vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;
    // link structures by pNext pointer
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &features12;

    // populate logical device create info
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    // enable features by pNext after vulkan 1.1
    createInfo.pNext = &deviceFeatures2;
    createInfo.pEnabledFeatures = VK_NULL_HANDLE;

    // enable device extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // add validation layer information
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(chosenGPU_, &createInfo, nullptr, &vkDevice_) != VK_SUCCESS)
        XE_CORE_ERROR("failed to create logical device!");
    
    // store the queue handle
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.graphics, 0, &graphicsQueue_);
    vkGetDeviceQueue(vkDevice_, queueFamilyIndices_.present, 0, &presentQueue_);

    // get extention functions' address
    VulkanExtentionFuncs::Init();
}

void RendererVulkan::CreateMemoryAllocator()
{
    VmaAllocatorCreateInfo vmaInfo = {};
    vmaInfo.physicalDevice = chosenGPU_;
    vmaInfo.device = vkDevice_;
    vmaInfo.instance = vkInstance_;
    vmaInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&vmaInfo, &vmaAllocator_);
}

void RendererVulkan::CreateFrameBuffers()
{
    // get screen's resolution as defalut draw resolution
    GLFWmonitor* monitor = Application::Instance().GetWindow().GetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    uint32_t width = mode->width;
    uint32_t height = mode->height;

    int resolutionHeight = 0, resolutionWidth = 0;
    glfwGetFramebufferSize(Application::Instance().GetWindow().GetNativeWindow(), &resolutionWidth, &resolutionHeight);
    if ((float)resolutionWidth / Application::Instance().GetWindow().GetWidth() != 1.f) {
        width *= (float)resolutionWidth / Application::Instance().GetWindow().GetWidth() ;
        height *= (float)resolutionWidth / Application::Instance().GetWindow().GetWidth() ;
    }
    
    // create color and depth frame buffer
    colorAttachment_ = CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
	depthAttachment_ = CreateDepthAttachment(width, height, VK_FORMAT_D32_SFLOAT);

    mainDeletionQueue_.push_deletor([&](){
		DestroyGpuImage(colorAttachment_);
		DestroyGpuImage(depthAttachment_);
	});
}

void RendererVulkan::CreateCommands()
{
    // create a command pool and a command buffer for the graphics queue for each FBO.
	// we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkutil::command_pool_create_info(queueFamilyIndices_.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
        // create command pool and command buffer for each fbo
		if (vkCreateCommandPool(vkDevice_, &commandPoolInfo, nullptr, &frameData_[i].commandPool) != VK_SUCCESS)
            XE_CORE_ERROR("RendererVulkan::CreateCommands() : Failed to create command pool")

		VkCommandBufferAllocateInfo cmdAllocInfo = vkutil::command_buffer_allocate_info(frameData_[i].commandPool, 1);
		if (vkAllocateCommandBuffers(vkDevice_, &cmdAllocInfo, &frameData_[i].mainCommandBuffer) != VK_SUCCESS)
            XE_CORE_ERROR("RendererVulkan::CreateCommands() : Fialed to create command buffer")
    }

	// create command pool and buffer for immediate submits
	vkCreateCommandPool(vkDevice_, &commandPoolInfo, nullptr, &immCommandPool_);
	VkCommandBufferAllocateInfo cmdAllocInfo = vkutil::command_buffer_allocate_info(immCommandPool_, 1);
	vkAllocateCommandBuffers(vkDevice_, &cmdAllocInfo, &immCommandBuffer_);
}

void RendererVulkan::CreateSyncObjects()
{
    VkFenceCreateInfo fenceCreateInfo = vkutil::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkutil::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vkCreateFence(vkDevice_, &fenceCreateInfo, nullptr, &frameData_[i].renderFence);
		vkCreateSemaphore(vkDevice_, &semaphoreCreateInfo, nullptr, &frameData_[i].swapchainSemaphore);
		vkCreateSemaphore(vkDevice_, &semaphoreCreateInfo, nullptr, &frameData_[i].renderSemaphore);
	}

	// immediate submit fence
	vkCreateFence(vkDevice_, &fenceCreateInfo, nullptr, &immFence_);
    
}

void RendererVulkan::CreateBuiltInDescriptorAllocators()
{
    // create descriptor pool for each frame data
	std::vector<DescriptorAllocator::PoolSizeRatio> GlobalPoolsizes = { 
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
	};

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        frameData_[i].frameDescriptorPool.Init(1000, GlobalPoolsizes);
    }

    // create the descriptor set layout for compute pipeline
	DescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	drawImageDescriptorLayout_  = builder.Build();

    // create global scene descriptor set layout
	builder.Clear();
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	gpuSceneDataDescriptorLayout_ = builder.Build();

    mainDeletionQueue_.push_deletor([&]() {
        // destroy builtin descriptor set layout and allocator
        vkDestroyDescriptorSetLayout(vkDevice_, drawImageDescriptorLayout_, nullptr);
        vkDestroyDescriptorSetLayout(vkDevice_, gpuSceneDataDescriptorLayout_, nullptr);
    });
}

void RendererVulkan::CreateBackGroundPipeLines()
{
	// Create Pipeline Layout
	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants) ;
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPipelineLayoutCreateInfo layoutCrateInfo{};
	layoutCrateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCrateInfo.pNext = nullptr;
	layoutCrateInfo.pSetLayouts = &drawImageDescriptorLayout_;
	layoutCrateInfo.setLayoutCount = 1;
	layoutCrateInfo.pPushConstantRanges = &pushConstant;
	layoutCrateInfo.pushConstantRangeCount = 1;

	VkPipelineLayout computeLayout;
	vkCreatePipelineLayout(vkDevice_, &layoutCrateInfo, nullptr, &computeLayout);

	// Create Pipeline
	VkShaderModule gradientShader = LoadShader("/Users/xieyhccc/develop/XEngine/src/Assets/Shaders/Spirv/gradient_color.comp.spv");
	VkShaderModule skyShader = LoadShader("/Users/xieyhccc/develop/XEngine/src/Assets/Shaders/Spirv/sky.comp.spv");
	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = gradientShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = computeLayout;
	computePipelineCreateInfo.stage = stageinfo;

	ComputeEffect gradient;
	gradient.computePipeline.layout = computeLayout;
	gradient.name = "gradient";
	gradient.data = {};
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);	//default colors
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	vkCreateComputePipelines(vkDevice_,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &gradient.computePipeline.pipeline);

	// create sky shader pipeline
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect sky;
	sky.computePipeline.layout = computeLayout;
	sky.name = "sky";
	sky.data = {};
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);	//default sky parameters

	vkCreateComputePipelines(vkDevice_, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.computePipeline.pipeline);

	// add the 2 background effects into the array
	backgroundEffects_.push_back(gradient);
	backgroundEffects_.push_back(sky);

	// destroy structures properly
    vkDestroyShaderModule(vkDevice_, gradientShader, nullptr);
    vkDestroyShaderModule(vkDevice_, skyShader, nullptr);

    mainDeletionQueue_.push_deletor([=]() {
        vkDestroyPipelineLayout(vkDevice_, computeLayout, nullptr);
        vkDestroyPipeline(vkDevice_,backgroundEffects_[0].computePipeline.pipeline, nullptr);
        vkDestroyPipeline(vkDevice_,backgroundEffects_[1].computePipeline.pipeline, nullptr);
    });
	
}

void RendererVulkan::DestroySwapChain()
{
	for (int i = 0; i < swapchainImageViews_.size(); i++) {
		vkDestroyImageView(vkDevice_, swapchainImageViews_[i], nullptr);
	}

    vkDestroySwapchainKHR(vkDevice_, swapChain_, nullptr);
}

void RendererVulkan::InitImgui()
{
    // 1: create descriptor pool for IMGUI
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 100;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	vkCreateDescriptorPool(vkDevice_, &pool_info, nullptr, &imguiPool);

    // 2: initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = "bin/imgui.ini";
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForVulkan(window_, true);

    // this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vkInstance_;
	init_info.PhysicalDevice = chosenGPU_;
	init_info.Device = vkDevice_;
	init_info.Queue = graphicsQueue_;
    init_info.QueueFamily = queueFamilyIndices_.graphics;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat_;

	ImGui_ImplVulkan_Init(&init_info);

    // add the destroy the imgui created structures
	mainDeletionQueue_.push_deletor([=]() {
		ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(vkDevice_, imguiPool, nullptr);
	});

}

void RendererVulkan::CreateDefaultResources()
{
	//3 default textures, white, grey, black. 1 pixel each
	constexpr uint32_t white = __builtin_bswap32(0xFFFFFFFF);
	whiteImage = CreateTexture((void*)&white, 1, 1, VK_FORMAT_R8G8B8A8_UNORM);

	constexpr uint32_t grey = __builtin_bswap32(0xFFAAAAAA);
	greyImage = CreateTexture((void*)&grey, 1, 1, VK_FORMAT_R8G8B8A8_UNORM);

	constexpr uint32_t black = __builtin_bswap32(0x000000FF);
	blackImage = CreateTexture((void*)&black, 1, 1, VK_FORMAT_R8G8B8A8_UNORM);

	//checkerboard image
	constexpr uint32_t magenta = __builtin_bswap32(0xFF00FFFF);
	std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	errorCheckerboardImage = CreateTexture(pixels.data(), 16, 16, VK_FORMAT_R8G8B8A8_UNORM);

    // create default samplers
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(vkDevice_, &sampl, nullptr, &defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(vkDevice_, &sampl, nullptr, &defaultSamplerLinear);

	mainDeletionQueue_.push_deletor([&](){
		DestroyGpuImage(whiteImage);
		DestroyGpuImage(greyImage);
		DestroyGpuImage(blackImage);
		DestroyGpuImage(errorCheckerboardImage);
		vkDestroySampler(vkDevice_, defaultSamplerLinear, nullptr);
		vkDestroySampler(vkDevice_, defaultSamplerNearest, nullptr);
	});
}
// -------------------------Utility functions for createing vulkan objects--------------------------
bool RendererVulkan::isPhysicalDeviceSuitable(VkPhysicalDevice device) const
{
    // get basic properties like name, type and supported vulkan version
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

#ifndef __APPLE__
    // require descrete gpu
    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return false;
#endif

    // check the support for optional features like texture compression, 64 bit floats and multi viewport rendering (useful for VR)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // require anisotropysample
    if (!deviceFeatures.samplerAnisotropy)
        return false;

    // query quefamily indices
    vkutil::QueueFamilyIndices indices = vkutil::GetQueueFamilyIndices(device, surface_);
    if (!indices.isComplete())
        return false;

    // get swapchain support details
    vkutil::SwapChainSupportDetails swapChainSupport = vkutil::GetSwapChainSupportDetails(device, surface_);
    // require at least one format and one present mode
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate)
        return false;

    // check device extension support
    if (!vkutil::CheckDeviceExtensionSupport(device, deviceExtensions))
        return false;

    return true;
}

VkSurfaceFormatKHR RendererVulkan::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // look for ideal format
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && (
            availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM || 
            availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM))
            return availableFormat;
    }

    // choose the first format if ideal format not available
    return availableFormats[0];
}

VkPresentModeKHR RendererVulkan::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
    // 在Vulkan中有四个模式可以使用:
    // 1，VK_PRESENT_MODE_IMMEDIATE_KHR
    // 应用程序提交的图像被立即传输到屏幕呈现，这种模式可能会造成撕裂效果。
    // 2，VK_PRESENT_MODE_FIFO_KHR
    // 交换链被看作一个队列，当显示内容需要刷新的时候，显示设备从队列的前面获取图像，并且程序将渲染完成的图像插入队列的后面。如果队列是满的程序会等待。这种规模与视频游戏的垂直同步很类似。显示设备的刷新时刻被称为“垂直中断”。
    // 3，VK_PRESENT_MODE_FIFO_RELAXED_KHR
    // 该模式与上一个模式略有不同的地方为，如果应用程序存在延迟，即接受最后一个垂直同步信号时队列空了，将不会等待下一个垂直同步信号，而是将图像直接传送。这样做可能导致可见的撕裂效果。
    // 4，VK_PRESENT_MODE_MAILBOX_KHR
    // 这是第二种模式的变种。当交换链队列满的时候，选择新的替换旧的图像，从而替代阻塞应用程序的情形。这种模式通常用来实现三重缓冲区，与标准的垂直同步双缓冲相比，它可以有效避免延迟带来的撕裂效果。

    // default mode
    VkPresentModeKHR defaultMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return defaultMode;
}

VkExtent2D RendererVulkan::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
{

    // if currentExtent equals std::numeric_limits<uint32_t>::max(), we can customize extent
    // otherwise we can only use given extent
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else {
        int width, height;
        glfwGetFramebufferSize(Application::Instance().GetWindow().GetNativeWindow(), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

std::vector<const char*> RendererVulkan::GetRequiredExtensions() const
{
    // get extensions for glfw
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

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;

}


void RendererVulkan::OnWindowResize(const WindowResizeEvent& event)
{
    ResizeSwapchain();

    XE_CORE_TRACE("ff");
}
