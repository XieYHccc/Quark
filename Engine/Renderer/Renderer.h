#pragma once
#include "Events/ApplicationEvent.h"
#include "Graphics/Vulkan/Context.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Assert.h"
#include "Graphics/Vulkan/Descriptor.h"
#include "Graphics/Vulkan/PipeLine.h"
#include "Renderer/DrawContext.h"

constexpr unsigned int FRAME_OVERLAP = 2;

struct GLFWwindow;

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct GpuDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

struct ComputeEffect {
    const char* name;
    vk::PipeLine computePipeline;
	ComputePushConstants data;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_deletor(std::function<void()>&& function) 
	{
		deletors.push_back(function);
	}

	void flush() 
	{
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
			(*it)(); //call functors
		deletors.clear();
	}
};

struct PerFrameData 
{	
    // current present swapchain image
    vk::SwapChainImage presentImage;

	// command buffer data
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	// sync data
	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;

	// descriptorset data per frame
	vk::DescriptorAllocator frameDescriptorPool;

	// deletion queue
	DeletionQueue deletionQueue;
};

class Scene;

/**
This class struct the basic rendering logic, which
is responsible for providing a PerFrameData for
per-frame buffer、descriptor creation and draw commands recording

This class also contains the general used descriptor 
set layout and graphics pipeline layout for simplifying 
the material and shader management.

we are also using a uber shader approach to simplify shader
and material management.
*/
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer instance;
		return instance;
	}

    void Init();
    void Finalize();

    PerFrameData* BeginFrame();
    void EndFrame();

    bool BeginRendering(VkCommandBuffer cmd, const VkRenderingInfo& info);
    void EndRendering();

    vk::Context& GetContext() { return vkContext_; }
    VkDevice GetVkDevice() { return vkDevice_; }
    VkFormat GetColorFormat() { return colorFormat_; }
    VkFormat GetDepthFormat() { return depthFormat_;}
    VkPipelineLayout GetGraphicsPipeLineLayout() { return graphicsPipelineLayout_; }
    VkDescriptorSetLayout GetSceneDescriptorSetLayout() { return sceneDataDescriptorLayout_; }
    VkExtent2D GetSwapCainExtent() { return vkContext_.GetSwapChainImageExtent(); }
    VkFormat GetSwapCainImageFormat() { return vkContext_.GetSwapChainImageFormat(); }

    VkDescriptorSet CreateMaterialDescriptorSet() { return materialDescriptorSetAllocator_.Allocate(materialDescriptorLayout_); }

    void DrawImgui(VkImageView targetView, VkExtent2D extent);
    void DrawBackGround(VkImageView targetView, VkExtent2D extent);

private:
    // initialize renderer
    // void CreateFrameBuffers();
    void CreateCommands();
    void CreateSyncObjects();
    void CreatePipeLineLayout();
    void CreateDescriptorAllocators();
    void CreateBackGroundPipeLines();
    void InitImgui();
    void CreateDefaultResources();

private:
    Renderer()  = default;
    ~Renderer()  = default;
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;

private:
    vk::Context vkContext_;
    VkDevice vkDevice_;

    // main deletion queue
    DeletionQueue mainDeletionQueue_;
    
    // color and depth attachments
    VkFormat colorFormat_ { VK_FORMAT_R16G16B16A16_SFLOAT };
    VkFormat depthFormat_ { VK_FORMAT_D32_SFLOAT };
    // vk::Image colorAttachment_;
    // vk::Image depthAttachment_;
    PerFrameData frameData_[FRAME_OVERLAP];
    VkExtent2D drawExtent_; // actual draw extent
    uint32_t currentFrame_;
    uint32_t currentPresentImage_;

    // compute pipelines struct
	VkDescriptorSetLayout drawImageDescriptorLayout_;
    ComputeEffect backgroundEffect_;

    // descriptor set layout and pipeline set layout for all graphic pipelines
    VkDescriptorSetLayout sceneDataDescriptorLayout_;
    VkDescriptorSetLayout materialDescriptorLayout_;
    vk::DescriptorAllocator materialDescriptorSetAllocator_;
    VkPipelineLayout graphicsPipelineLayout_;


};


