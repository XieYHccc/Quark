#pragma once
#include "Events/ApplicationEvent.h"
#include "pch.h"

#include "Graphics/Vulkan/VulkanTypes.h"
#include "Graphics/Vulkan/VulkanInitUtils.h"
#include "Graphics/Vulkan/DescriptorVulkan.h"
#include "Graphics/Vulkan/PipelineBuilder.h"
#include "Graphics/Vulkan/MaterialVulkan.h"

constexpr unsigned int FRAME_OVERLAP = 2;

struct GLFWwindow;

struct PerFrameData 
{	
	// command buffer data
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	// sync data
	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;

	// descriptorset data per frame
	DescriptorAllocator frameDescriptorPool;

	// deletion queue
	vkutil::DeletionQueue deletionQueue;
};

// A logical gpu render device
class RendererVulkan {
public:
    static void Creat();
    static RendererVulkan* GetInstance() { return instance_; }

private:
    static RendererVulkan* instance_;

public:
    void Initialize();
    void Finalize();


    // ----------------------------Render Frame API------------------------------

    // all draw commands happens between BeginFrame() and EndFrame()
    void BeginFrame();
    void EndFrame();

    void DrawGeometry(VkImageView colorTargetView, VkImageView depthTargetView, VkExtent2D extent, const DrawContext& context);
    void DrawImgui(VkImageView targetView, VkExtent2D extent);
    void DrawBackGround(VkImageView targetView, VkExtent2D extent);

    void TransitionImageLayout(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
    void CopyImagetoImage(VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
    
    PerFrameData GetCurrentFrameData() { return frameData_[currentFrame_]; }
    VkImage GetCurrentPresentImage() { return swapChainImages_[currentPresentImage_];}
    VkImageView GetCurrentPresentImageView() { return swapchainImageViews_[currentPresentImage_];}
    VkExtent2D GetSwapCainExtent() { return swapChainExtent_; }
    VkDevice GetVkDevice() { return vkDevice_; }

    VkFormat GetSwapCainImageFormat() { return swapChainImageFormat_; }
    VkDescriptorSetLayout GetSceneDescriptorSetLayout() const { return gpuSceneDataDescriptorLayout_;}

    // -----------------------Gpu Resources Creation API--------------------
    UniformBuffer CreateUniformBuffer(size_t size);
    VertexBuffer CreateVertexBuffer(std::span<Vertex> vertices);
    IndexBuffer CreateIndexBuffer(std::span<uint32_t> indices);
    GpuMeshBuffers CreateMeshBuffers(std::span<uint32_t> indices, std::span<Vertex> vertices);
    Texture CreateTexture(void* data, uint32_t width, uint32_t height, bool mipmapped);
    DepthAttachment CreateDepthAttachment(uint32_t width, uint32_t height, VkFormat format);
    ColorAttachment CreateColorAttachment(uint32_t width, uint32_t height, VkFormat format);
    VkShaderModule LoadShader(const char* filePath);

    // lower level
    AllocatedImage CreateVulkanImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedBuffer CreateVulkanBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    // destroy resources on GPU
    void DestroyGpuBuffer(const AllocatedBuffer& buffer);
    void DestroyGpuImage(const AllocatedImage& image);
    
    // --------------------------evnet callback function-------------------
    // evnet callback functions
    void OnWindowResize(const WindowResizeEvent& event);

private:
    // initialize render device
    void CreateVkInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void PickGPU();
    void CreateLogicalDevice();
    void CreateMemoryAllocator();
	void CreateSwapChain();
    void CreateFrameBuffers();
    void CreateCommands();
    void CreateSyncObjects();
    void CreateBuiltInDescriptorAllocators();
    void CreateBackGroundPipeLines();
    void ResizeSwapchain();
    void InitImgui();
    void CreateDefaultResources();

    void DestroySwapChain();

    // utility functions for creating vulkan objects
    std::vector<const char*> GetRequiredExtensions() const;
    bool isPhysicalDeviceSuitable(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablemodes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
    RendererVulkan() {};

public:
    // default textures and samplers
    AllocatedImage whiteImage;
	AllocatedImage blackImage;
	AllocatedImage greyImage;
	AllocatedImage errorCheckerboardImage;
    VkSampler defaultSamplerLinear;
	VkSampler defaultSamplerNearest;

    // default material factories
    GLTFMetallic_Roughness metalRoughMaterial;

private:

    // glfw window
    GLFWwindow* window_;
    VkExtent2D windowExtent_;
    // Vulkan objects
    VkInstance vkInstance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice chosenGPU_;
    vkutil::QueueFamilyIndices queueFamilyIndices_;
    VkDevice vkDevice_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VmaAllocator vmaAllocator_;
    VkSwapchainKHR swapChain_;
    std::vector<VkImage> swapChainImages_;
	std::vector<VkImageView> swapchainImageViews_;
	VkExtent2D swapChainExtent_;
    VkFormat swapChainImageFormat_;

    // immediate submit structures
    VkFence immFence_;
    VkCommandBuffer immCommandBuffer_;
    VkCommandPool immCommandPool_;

    // main deletion queue
    vkutil::DeletionQueue mainDeletionQueue_;
    
    // color and depth attachments
    VkFormat colorAttachmentFormat_;
    ColorAttachment colorAttachment_;
    DepthAttachment depthAttachment_;
    PerFrameData frameData_[FRAME_OVERLAP];
    VkExtent2D drawExtent_; // actual draw extent
    uint32_t currentFrame_;
    uint32_t currentPresentImage_;

    // computing background pipeline objects
	VkDescriptorSetLayout drawImageDescriptorLayout_;
    std::vector<ComputeEffect> backgroundEffects_;
	int currentBackgroundEffect_;

    // scene data
    VkDescriptorSetLayout gpuSceneDataDescriptorLayout_;

};


