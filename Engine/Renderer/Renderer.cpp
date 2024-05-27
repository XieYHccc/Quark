#include "pch.h"
#include "Renderer/Renderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Core/Window.h"
#include "Graphics/Vulkan/Initializers.h"
#include "Graphics/Vulkan/Buffer.h"
#include "Events/EventManager.h"

void Renderer::Init()
{
	// Init vulkan context
	vkContext_.Init();
    vkDevice_ = vkContext_.GetVkDevice();

	// Create objects for vulkan commands
    CreateCommands();
    CreateSyncObjects();
	CreatePipeLineLayout();
    CreateDescriptorAllocators();
    CreateBackGroundPipeLines();
    CreateDefaultResources();
    InitImgui();

    currentFrame_ = 0;

    CORE_LOGI("Vulkan Renderer Created")

}

void Renderer::Finalize()
{
    vkDeviceWaitIdle(vkDevice_);

    mainDeletionQueue_.flush();

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

	// Destroy vulkan context
    vkContext_.Finalize();
}

PerFrameData* Renderer::BeginFrame()
{
    // wait until the gpu has finished rendering the last frame.
    vkWaitForFences(vkDevice_, 1, &frameData_[currentFrame_].renderFence, true, 1000000000);
	vkResetFences(vkDevice_, 1, &frameData_[currentFrame_].renderFence);

    // request image from the swapchain
	VkResult e = vkAcquireNextImageKHR(vkDevice_, vkContext_.GetVkSwapChain(), 1000000000, frameData_[currentFrame_].swapchainSemaphore, nullptr, &currentPresentImage_);
    if (e == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkContext_.ResizeSwapchain();
		return VK_NULL_HANDLE;
	}
    else if (e != VK_SUCCESS && e != VK_SUBOPTIMAL_KHR)
        CORE_LOGE("failed to acquire swap chain image!");

    // refresh frame data
    frameData_[currentFrame_].deletionQueue.flush();
    frameData_[currentFrame_].frameDescriptorPool.ClearPools();
	frameData_[currentFrame_].presentImage = vkContext_.GetSwapChainImage(currentPresentImage_);

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBuffer cmd = frameData_[currentFrame_].mainCommandBuffer;
	vkResetCommandBuffer(cmd, 0);
	VkCommandBufferBeginInfo cmdBeginInfo = vk::init::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(cmd, &cmdBeginInfo);

	return &frameData_[currentFrame_];

}

void Renderer::EndFrame()
{
    VkCommandBuffer cmd = frameData_[currentFrame_].mainCommandBuffer;
    
    vkEndCommandBuffer(cmd);

    // prepare the submission to the queue. 
	VkCommandBufferSubmitInfo cmdinfo = vk::init::command_buffer_submit_info(cmd);	
	VkSemaphoreSubmitInfo waitInfo = vk::init::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frameData_[currentFrame_].swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vk::init::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frameData_[currentFrame_].renderSemaphore);	
	VkSubmitInfo2 submit = vk::init::submit_info(&cmdinfo,&signalInfo,&waitInfo);	
	vkContext_.extendFunction.pVkQueueSubmit2KHR(vkContext_.GetGraphicQueue(), 1, &submit, frameData_[currentFrame_].renderFence);
 
    // prepare present
    VkSwapchainKHR swapchain = vkContext_.GetVkSwapChain();
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &frameData_[currentFrame_].renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &currentPresentImage_;
	VkResult presentResult = vkQueuePresentKHR(vkContext_.GetPresentQueue(), &presentInfo);

    if (presentResult != VK_SUCCESS && presentResult != VK_ERROR_OUT_OF_DATE_KHR
		&& presentResult != VK_SUBOPTIMAL_KHR) {
        CORE_ASSERT_MSG(false, "failed to present swap chain image!")
    }

    currentFrame_  = (currentFrame_ + 1) % FRAME_OVERLAP;
}

void Renderer::DrawBackGround(VkImageView targetView, VkExtent2D extent)
{
    PerFrameData& currentFrame = frameData_[currentFrame_];
    
    VkDescriptorSet drawImageDescriptor = currentFrame.frameDescriptorPool.Allocate(drawImageDescriptorLayout_);

    vk::DescriptorWriter writer;
	writer.WriteImage(0, targetView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(vkContext_, drawImageDescriptor);

    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;

	// bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, backgroundEffect_.computePipeline.pipeline);
	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, backgroundEffect_.computePipeline.layout, 0, 1, &drawImageDescriptor, 0, nullptr);
	// bind push constants 
	vkCmdPushConstants(cmd, backgroundEffect_.computePipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &backgroundEffect_.data);
	// execute the compute  pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);
}

void Renderer::DrawImgui(VkImageView targetView, VkExtent2D extent)
{
    // determine draw extent

    VkCommandBuffer cmd = frameData_[currentFrame_].mainCommandBuffer;
	VkRenderingAttachmentInfo colorAttachment = vk::init::attachment_info(targetView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingInfo renderInfo = vk::init::rendering_info(extent, &colorAttachment, nullptr);

	vkContext_.extendFunction.pVkCmdBeginRenderingKHR(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkContext_.extendFunction.pVkCmdEndRenderingKHR(cmd);
}


// void Renderer::CreateFrameBuffers()
// {
    
//     u32 width = vkContext_.GetSwapChainImageExtent().width;
//     u32 height = vkContext_.GetSwapChainImageExtent().height;

// 	// create draw image
// 	vk::ImageBuilder builder;
// 	builder.SetExtent(VkExtent3D{width, height, 1})
// 		.SetUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
// 			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
// 		.SetFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
// 		.SetVmaUsage(VMA_MEMORY_USAGE_GPU_ONLY)
// 		.SetVmaRequiredFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//     colorAttachment_ = builder.Build(vkContext_);

// 	// create depth image
// 	builder.SetUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
// 		.SetFormat(VK_FORMAT_D32_SFLOAT);
// 	depthAttachment_ = builder.Build(vkContext_);

//     mainDeletionQueue_.push_deletor([&](){
// 		vk::Image::DestroyImage(vkContext_, colorAttachment_);
// 		vk::Image::DestroyImage(vkContext_, depthAttachment_);
// 	});
// }

void Renderer::CreateCommands()
{
    // create a command pool and a command buffer for the graphics queue for each FBO.
	// we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vk::init::command_pool_create_info(vkContext_.GetQueueFamilyIndices().graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
        // create command pool and command buffer for each fbo
		if (vkCreateCommandPool(vkDevice_, &commandPoolInfo, nullptr, &frameData_[i].commandPool) != VK_SUCCESS)
            CORE_ASSERT_MSG(false, "Renderer::CreateCommands() : Failed to create command pool")

		VkCommandBufferAllocateInfo cmdAllocInfo = vk::init::command_buffer_allocate_info(frameData_[i].commandPool, 1);
		if (vkAllocateCommandBuffers(vkDevice_, &cmdAllocInfo, &frameData_[i].mainCommandBuffer) != VK_SUCCESS)
            CORE_ASSERT_MSG(false, "Renderer::CreateCommands() : Fialed to create command buffer")
    }

}

void Renderer::CreateSyncObjects()
{
    VkFenceCreateInfo fenceCreateInfo = vk::init::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk::init::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vkCreateFence(vkDevice_, &fenceCreateInfo, nullptr, &frameData_[i].renderFence);
		vkCreateSemaphore(vkDevice_, &semaphoreCreateInfo, nullptr, &frameData_[i].swapchainSemaphore);
		vkCreateSemaphore(vkDevice_, &semaphoreCreateInfo, nullptr, &frameData_[i].renderSemaphore);
	}
    
}

void Renderer::CreatePipeLineLayout()
{
    // create the descriptor set layout for compute pipeline
	vk::DescriptorSetLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	drawImageDescriptorLayout_  = builder.Build(vkContext_);

    // create global scene descriptor set layout
	builder.Clear();
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	sceneDataDescriptorLayout_ = builder.Build(vkContext_);

	// create material descriptor set layout
	builder.Clear();
    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	builder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    materialDescriptorLayout_ = builder.Build(vkContext_);

	// create pipeline push constants range
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GpuDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// create graphic pipeline layout
	VkDescriptorSetLayout layouts[] = { sceneDataDescriptorLayout_, materialDescriptorLayout_ };

	VkPipelineLayoutCreateInfo mesh_layout_info = vk::init::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VK_ASSERT(vkCreatePipelineLayout(vkDevice_, &mesh_layout_info, nullptr, &graphicsPipelineLayout_));

    mainDeletionQueue_.push_deletor([&]() {
        // destroy builtin descriptor set layout and allocator
        vkDestroyDescriptorSetLayout(vkDevice_, drawImageDescriptorLayout_, nullptr);
        vkDestroyDescriptorSetLayout(vkDevice_, sceneDataDescriptorLayout_, nullptr);
		vkDestroyDescriptorSetLayout(vkDevice_, materialDescriptorLayout_, nullptr);
		vkDestroyPipelineLayout(vkDevice_, graphicsPipelineLayout_, nullptr);
    });
}

void Renderer::CreateDescriptorAllocators()
{
    // create descriptor pool for each frame data
	std::vector<vk::DescriptorAllocator::PoolSizeRatio> globalPoolSizes = { 
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
	};

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        frameData_[i].frameDescriptorPool.Init(vkContext_, 100, globalPoolSizes);
    }

	std::vector<vk::DescriptorAllocator::PoolSizeRatio> matPoolSizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

    materialDescriptorSetAllocator_.Init(vkContext_, 100, matPoolSizes);

	mainDeletionQueue_.push_deletor([&]() {
		materialDescriptorSetAllocator_.DestroyPools();
	});
}

void Renderer::CreateBackGroundPipeLines()
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
	VkShaderModule gradientShader;
	vk::LoadShaderModule("../Assets/Shaders/Spirv/gradient_color.comp.spv", vkDevice_, &gradientShader);

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

	backgroundEffect_.computePipeline.layout = computeLayout;
	backgroundEffect_.name = "gradient";
	backgroundEffect_.data = {};
	backgroundEffect_.data.data1 = glm::vec4(1, 0, 0, 1);	//default colors
	backgroundEffect_.data.data2 = glm::vec4(0, 0, 1, 1);

	vkCreateComputePipelines(vkDevice_,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &backgroundEffect_.computePipeline.pipeline);

	// destroy structures properly
    vkDestroyShaderModule(vkDevice_, gradientShader, nullptr);

    mainDeletionQueue_.push_deletor([=]() {
        vkDestroyPipelineLayout(vkDevice_, computeLayout, nullptr);
        vkDestroyPipeline(vkDevice_, backgroundEffect_.computePipeline.pipeline, nullptr);
    });
	
}

void Renderer::InitImgui()
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
    ImGui::GetIO().IniFilename = "bin/log/imgui.ini";
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForVulkan(vkContext_.GetGLFWWindow(), true);

    // this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vkContext_.GetVkInstance();
	init_info.PhysicalDevice = vkContext_.GetVkPhysicalDevice();
	init_info.Device = vkDevice_;
	init_info.Queue = vkContext_.GetGraphicQueue();
    init_info.QueueFamily = vkContext_.GetQueueFamilyIndices().graphics;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat swapChainFormat = vkContext_.GetSwapChainImageFormat();
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainFormat;

	ImGui_ImplVulkan_Init(&init_info);

    // add the destroy the imgui created structures
	mainDeletionQueue_.push_deletor([=]() {
		ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(vkDevice_, imguiPool, nullptr);
	});

}

void Renderer::CreateDefaultResources()
{
	//3 default textures, white, black. 1 pixel each
	constexpr uint32_t white = __builtin_bswap32(0xFFFFFFFF);
	asset::TextureCreateInfo info{(unsigned char*)&white, 1, 1,
		4, false, "BuiltIn", "WhiteTexture"};
	auto whiteTex = asset::Texture::AddToPool(info);
	CORE_DEBUG_ASSERT(whiteTex != nullptr)

	// Black texture
	constexpr uint32_t black = __builtin_bswap32(0x000000FF);
	info.data = (unsigned char*)&black;
	info.name = "BlackTexture";
	asset::Texture::AddToPool(info);

	// errorcheck image
	constexpr uint32_t magenta = __builtin_bswap32(0xFF00FFFF);
	std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	info.data = (unsigned char*)&pixels;
	info.width = 16;
	info.height = 16;
	info.name = "ErrorTexture";
	auto errTex = asset::Texture::AddToPool(info);
	CORE_DEBUG_ASSERT(errTex != nullptr)

    // create default nearest sampler
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;
	asset::SamplerCreateInfo samplinfo {sampl, "BuiltIn", "DefaultNearestSampler"};
	asset::Sampler::AddToPool(samplinfo);

	// default linear sampler
	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	samplinfo.name = "DefaultLinearSampler";
	auto linearSampler = asset::Sampler::AddToPool(samplinfo);

	// default material
	asset::MaterialCreateInfo matInfo;
	matInfo.baseColorTex.texture = errTex;
	matInfo.baseColorTex.sampler = linearSampler;
	matInfo.metalRougthTex.texture = whiteTex;
	matInfo.metalRougthTex.sampler = linearSampler;
	matInfo.bufferData.colorFactors = glm::vec4(1.f);
	matInfo.bufferData.metalRoughFactors = glm::vec4(0.f, 0.5f, 0.f, 0.f);
	matInfo.mode = AlphaMode::OPAQUE;
	matInfo.path = "BuiltIn";
	matInfo.name = "DefaultMaterial";

	asset::Material::AddToPool(matInfo);

}

