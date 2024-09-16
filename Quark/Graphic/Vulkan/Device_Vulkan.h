#pragma once
#include "Quark/Graphic/Device.h"
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/Vulkan/Buffer_Vulkan.h"
#include "Quark/Graphic/Vulkan/Image_Vulkan.h"
#include "Quark/Graphic/Vulkan/Context_Vulkan.h"
#include "Quark/Graphic/Vulkan/CommandList_Vulkan.h"
#include "Quark/Graphic/Vulkan/PipeLine_Vulkan.h"
#include "Quark/Graphic/Vulkan/DescriptorSetAllocator.h"

namespace quark::graphic {

class Device_Vulkan final: public Device {
public:
    // Mostly for static data uploading with dedicated transfer queue
    class CopyCmdAllocator {
    public:
        struct CopyCmd 
        {   // command buffer for data transfering
            VkCommandPool transferCmdPool = VK_NULL_HANDLE;
            VkCommandBuffer transferCmdBuffer = VK_NULL_HANDLE;

            // command buffer for image layout transitioning and image blitting
            VkCommandPool transitionCmdPool = VK_NULL_HANDLE;
            VkCommandBuffer transitionCmdBuffer = VK_NULL_HANDLE;

            Ref<Buffer> stageBuffer = nullptr;

            VkSemaphore semaphores[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
            VkFence fence = VK_NULL_HANDLE;

            bool isValid() const { return transferCmdBuffer && transitionCmdBuffer; }
        };

        void init(Device_Vulkan* device);
        void destroy();
        CopyCmd allocate(VkDeviceSize required_buffer_size);
        void submit(CopyCmd cmd);

    private:
        Device_Vulkan* m_Device;
        std::vector<CopyCmd> m_FreeList;
        std::mutex m_Locker;
    };

    struct PerFrameData {
        Device_Vulkan* device = nullptr;
        VmaAllocator vmaAllocator = nullptr;
        std::vector<CommandList_Vulkan*> cmdLists[QUEUE_TYPE_MAX_ENUM];
        u32 cmdListCount[QUEUE_TYPE_MAX_ENUM] = {}; //  The count of cmd used in this frame. Cleared when a new frame begin
        VkFence queueFences[QUEUE_TYPE_MAX_ENUM];   // Per queue fence. Signled when all command list submitted from this frame completed.
        std::vector<VkFence> waitedFences;

        VkSemaphore imageAvailableSemaphore;
        VkSemaphore imageReleaseSemaphore;
        bool imageAvailableSemaphoreConsumed = false;

        std::vector<std::pair<VkBuffer, VmaAllocation>> garbageBuffers;
        std::vector<std::pair<VkImage, VmaAllocation>> garbageImages;
        std::vector<VkPipeline> garbagePipelines;
        std::vector<VkImageView> grabageViews;
        std::vector<VkShaderModule> garbageShaderModules;
        std::vector<VkSampler> garbageSamplers;

        void init(Device_Vulkan* device);
        void reset();   // Reset this frame
        void destroy();
        void clear();   // Deferred destroy of resources that gpu is already finished with
    };

public:
    static constexpr uint8_t MAX_FRAME_NUM_IN_FLIGHT = 2;
    VkDevice vkDevice; // Borrowed from context, no lifetime management here
    VmaAllocator vmaAllocator; // Borrowed from context, no lifetime management here
    Scope<VulkanContext> vkContext;
    CopyCmdAllocator copyAllocator;

    // Cached objects
    std::unordered_map<size_t, PipeLineLayout> cached_pipelineLayouts;
    std::unordered_map<size_t, DescriptorSetAllocator> cached_descriptorSetAllocator;

public:
    Device_Vulkan() = default;
    ~Device_Vulkan() = default;
    
    bool Init() override final;
    void ShutDown() override final;
    bool BeiginFrame(TimeStep ts) override final;
    bool EndFrame(TimeStep ts) override final;
    void OnWindowResize(const WindowResizeEvent& event) override final;

    /*** RESOURCES ***/
    Ref<Buffer> CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override final;
    Ref<Image> CreateImage(const ImageDesc& desc, const ImageInitData* init_data = nullptr) override final;
    Ref<Shader> CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize) override final;
    Ref<Shader> CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path) override final;
    Ref<PipeLine> CreateGraphicPipeLine(const GraphicPipeLineDesc& desc) override final;
    Ref<Sampler> CreateSampler(const SamplerDesc& desc) override final;

    /*** COMMAND LIST ***/
    CommandList* BeginCommandList(QueueType type = QueueType::QUEUE_TYPE_GRAPHICS) override final;
    void SubmitCommandList(CommandList* cmd, CommandList* waitedCmds = nullptr, uint32_t waitedCmdCounts = 0, bool signal = false) override final;

    Image* GetPresentImage() override final { return m_SwapChainImages[m_CurrentSwapChainImageIdx].get(); }
    DataFormat GetPresentImageFormat() override final;

    bool isFormatSupported(DataFormat format) override final;
    void SetDebugName(const Ref<GpuResource>& resouce, const char* name) override final;

    ///////////////////// Vulkan specific ////////////////////////
    //////////////////////////////////////////////////////////////
public:
    DescriptorSetAllocator* Request_DescriptorSetAllocator(const DescriptorSetLayout& layout);

    PipeLineLayout* Request_PipeLineLayout(const ShaderResourceLayout& combinedLayout);

    PerFrameData& GetCurrentFrame() { return m_Frames[m_elapsedFrame % MAX_FRAME_NUM_IN_FLIGHT]; }

private:
    void ResizeSwapchain();

    // Represent a physical queue
    // Responsible for queuing commad buffers and submit them in batch
    struct CommandQueue 
    {
        // represent a VkSubmitInfo2
        struct Submitssion
        {
            std::vector<VkCommandBufferSubmitInfo> cmdInfos;
            std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
            std::vector<VkSemaphoreSubmitInfo> signalSemaphoreInfos;
        };

        Device_Vulkan* device = nullptr;
        QueueType type = QueueType::QUEUE_TYPE_MAX_ENUM;
        VkQueue queue = VK_NULL_HANDLE;
        std::vector<Submitssion> submissions;
        std::mutex locker;

        void init(Device_Vulkan* device, QueueType type);
        void submit(VkFence fence = nullptr);
    };

    std::vector<Ref<Image>> m_SwapChainImages; // Owned by Context' swapchain, no lifetime management here
    uint32_t m_CurrentSwapChainImageIdx;

    PerFrameData m_Frames[MAX_FRAME_NUM_IN_FLIGHT];
    CommandQueue m_Queues[QUEUE_TYPE_MAX_ENUM];

    bool m_RecreateSwapchain;

};
}