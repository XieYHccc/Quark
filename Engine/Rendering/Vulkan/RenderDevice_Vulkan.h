#pragma once
#include "Rendering/RenderDevice.h"
#include "Rendering/Vulkan/VulkanAssert.h"
#include "Rendering/Vulkan/VulkanDevice.h"
#include "Rendering/Vulkan/VulkanSwapChain.h"

class RenderDevice_Vulkan final : public RenderDevice {
public:
    virtual bool Init() override final;
    virtual void ShutDown() override final;
    virtual void OnWindowResize(const WindowResizeEvent& event) override final;

	/*****************/
	/**** IMAGE ****/
	/*****************/
private:
    struct GPUImage_Vulkan {
        VkImage handle = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        struct {
            VmaAllocation handle = nullptr;
            VmaAllocationInfo info = {};
        } allocation;
    };
public:

    virtual void ImageCreate(const GPUImageDesc& desc, GPUImage* outImage) override final;
    virtual void ImageFree(GPUImage* image) override final;

    /*****************/
	/***** Shader ****/
	/*****************/
private:
	struct Shader_Vulkan {
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo stageInfo = {};
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		VkPushConstantRange pushconstants = {};
        VkPipeline pipeline_cs = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout_cs = VK_NULL_HANDLE; 
    };

public:
    virtual void ShaderCreateFromBytes(ShaderStage stage, void* byteCode, size_t byteCount, Shader* outShader) override final;
    virtual bool ShaderCreateFromSpvFile(ShaderStage stage, const std::string& name, Shader* outShader) override final;
    virtual void ShaderFree(Shader* shader) override final;
    /******************************/
	/**** PipeLine State Object ***/
	/******************************/
private:
    struct PipeLine_Vulkan {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // no lifetime management here
        std::vector<VkDescriptorSetLayout> setLayout;
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutBindings;
        VkPushConstantRange pushconstants = {};
    };

public:
    virtual bool GraphicPipeLineCreate(PipeLineDesc& desc, PipeLine* outPSO) override final;
    virtual bool ComputePipeLineCreate(Shader* compShader, PipeLine* outPSO) override final;
    virtual void PipeLineFree(PipeLine* pipeline) override final;

    /********************/
	/*** Command List ***/
	/********************/
private:
    struct CommandList_Vulkan {
        VkSemaphore cmdCompleteSemaphore;
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        VkCommandPool cmdPool = VK_NULL_HANDLE;
        QueueType type = QUEUE_TYPE_MAX_ENUM;
        std::vector<CommandList> waitForCmds;
        std::vector<VkImageMemoryBarrier2> renderPassBeginBarriers;   // For graphics command listsx
        std::vector<VkImageMemoryBarrier2> renderPassEndBarriers;
        std::vector<VkMemoryBarrier2> memoryBarriers;
        std::vector<VkImageMemoryBarrier2> imageBarriers;
        std::vector<VkBufferMemoryBarrier2> bufferBarriers;
        bool waitForSwapchainImage = false;
        bool beWaited { false };

    };

    CommandList_Vulkan& GetInternalCommanddList(CommandList cmd) { return *((CommandList_Vulkan*)cmd.internal); }

public:
    virtual CommandList CommandListBegin(QueueType type = QUEUE_TYPE_GRAPHICS) override final;
    virtual void CommandListEnd(CommandList cmd) override final;
    // virtual void CommandListWait (CommandList cmd, CommandList waitCmd) override final;

    /*************/
	/*** FRAME ***/
	/*************/
private: 
    struct PerFrameData {
        // TODO: Support Multithreading
        struct QueueCommand
        {
            std::vector<CommandList_Vulkan> cmds;
            u32 cmdsCount = 0;
            VkFence inFlghtfence = VK_NULL_HANDLE;
        };
        
        QueueCommand queueCommands[QUEUE_TYPE_MAX_ENUM];
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderCompleteSemaphore;
        bool imageAvailableSemaphoreConsumed;
    };

public:
    virtual bool BeiginFrame(f32 deltaTime) override final;
    virtual bool EndFrame(f32 deltaTime) override final;

    /*****************/
	/*** RENDERING ***/
	/*****************/
private:
    struct GpuDrawPushConstants {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

public:
    virtual void CmdBeginRenderPass(CommandList cmd, RenderPass& renderPass) override final;
    virtual void CmdEndRenderPass(CommandList cmd) override final;
    virtual void CmdPipeLineBarriers(CommandList cmd, PipelineMemoryBarrier* memoryBarriers, u32 memoryBarriersCount, PipelineImageBarrier* iamgeBarriers, u32 iamgeBarriersCount, PipelineBufferBarrier* bufferBarriers, u32 bufferBarriersCount) override final;

    /*****************/
	/*** SWAPCHAIN ***/
	/*****************/
    virtual GPUImage* GetSwapChainImage() override final;

private:
    void CreateVulkanInstance();
    void CreateDebugMessenger();
    void CreateVulkanSurface();
    void CreatePipeLineLayout();
    void CreateFrameData();
    void CreateCopyCmd();
    void InitExtendFunctions();

    /*****************/
	/**** HELPERS ****/
	/*****************/
    GPUImage_Vulkan& GetInternal(const GPUImage* image) const { return *static_cast<GPUImage_Vulkan*>(image->internal); }
    CommandList_Vulkan& GetInternal(CommandList cmdList) const { return *static_cast<CommandList_Vulkan*>(cmdList.internal); }

    // Get required vulkan instance extensions
    std::vector<const char*> GetRequiredExtensions() const;
    // Debug callback function，VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
    static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

public:
    friend class VulkanSwapChain;
    static constexpr u8 MAX_FRAME_NUM_IN_FLIGHT = 2;
    VkInstance vkInstance;
#ifdef QK_DEBUG_BUILD
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
    VkSurfaceKHR vkSurface;
    VulkanDevice* device;
    VulkanSwapChain* swapChain;

    VkDescriptorSetLayout sceneDataDescriptorLayout;
    VkDescriptorSetLayout materialDescriptorLayout;
    VkPipelineLayout graphicPipeLineLayout;

    PerFrameData frames[MAX_FRAME_NUM_IN_FLIGHT];
    u32 currentSwapChainImageIdx;

    // For uploading static read-only data
    struct CopyCmd {
        // After submitting, cpu need to wait for the fence, which would block cpu.
        // TODO: support async uploading. 
        VkFence fence;
        VkCommandPool cmdPool;
        VkCommandBuffer cmdBuffer;
    } copyCmd;

    bool recreateSwapchain;

    // Extend functions
    struct ExtendFunction {
        PFN_vkQueueSubmit2KHR pVkQueueSubmit2KHR;
        PFN_vkCmdBlitImage2KHR pVkCmdBlitImage2KHR;
        PFN_vkCmdPipelineBarrier2KHR pVkCmdPipelineBarrier2KHR;
        PFN_vkCmdBeginRenderingKHR pVkCmdBeginRenderingKHR;
        PFN_vkCmdEndRenderingKHR pVkCmdEndRenderingKHR;
    } extendFunctions;

};
