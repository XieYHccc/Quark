#pragma once
#include "Quark/RHI/Device.h"
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Vulkan/Buffer_Vulkan.h"
#include "Quark/RHI/Vulkan/Image_Vulkan.h"
#include "Quark/RHI/Vulkan/Context_Vulkan.h"
#include "Quark/RHI/Vulkan/CommandList_Vulkan.h"
#include "Quark/RHI/Vulkan/PipeLine_Vulkan.h"
#include "Quark/RHI/Vulkan/DescriptorSetAllocator.h"

#include <atomic>

namespace quark::rhi {

struct PerFrameContext
{
    Device_Vulkan* device = nullptr;
    VmaAllocator vmaAllocator = nullptr;
    std::vector<CommandList_Vulkan*> cmdLists[QUEUE_TYPE_MAX_ENUM];
    uint32_t cmdListCount[QUEUE_TYPE_MAX_ENUM] = {}; //  the count of cmd used in this frame. Cleared when a new frame begin
    VkFence queueFences[QUEUE_TYPE_MAX_ENUM];   // per queue fence. Signled when all command list submitted from this frame completed.
    std::vector<VkFence> waitedFences;

    std::vector<std::pair<VkBuffer, VmaAllocation>> garbage_buffers;
    std::vector<std::pair<VkImage, VmaAllocation>> garbage_images;
    std::vector<VkPipeline> garbage_pipelines;
    std::vector<VkImageView> grabage_views;
    std::vector<VkShaderModule> garbage_shaderModules;
    std::vector<VkSampler> garbage_samplers;
    std::vector<BufferBlock> ubo_blocks;

    void init(Device_Vulkan* device);
    void begin();   // reset this frame
    void destroy();
    void clear();   // deferred destroy of resources that gpu is already finished with

};

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
    Device_Vulkan* m_device;
    std::vector<CopyCmd> m_freeList;
    std::mutex m_locker;
};

class Device_Vulkan final: public Device {
    friend class PerFrameContext;
    friend class CopyCmdAllocator;

public:
    VkDevice vkDevice; // norrowed from context, no lifetime management here
    VmaAllocator vmaAllocator; // borrowed from context, no lifetime management here
    CopyCmdAllocator copyAllocator;

    // Cached objects
    std::unordered_map<size_t, PipeLineLayout> cached_pipelineLayouts;
    std::unordered_map<size_t, DescriptorSetAllocator> cached_descriptorSetAllocator;

public:
    Device_Vulkan(const DeviceConfig& config);
    virtual ~Device_Vulkan();
    
    bool BeiginFrame(TimeStep ts) override final;
    bool EndFrame(TimeStep ts) override final;
    void OnWindowResize(const WindowResizeEvent& event) override final;
    void CopyBuffer(Buffer& dst, Buffer& src, uint64_t size, uint64_t dstOffset = 0, uint64_t srcOffset = 0) override final;

    /*** RESOURCES ***/
    Ref<Buffer>         CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override final;
    Ref<Image>          CreateImage(const ImageDesc& desc, const ImageInitData* init_data = nullptr) override final;
    Ref<Shader>         CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize) override final;
    Ref<Shader>         CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path) override final;
    Ref<PipeLine>       CreateGraphicPipeLine(const GraphicPipeLineDesc& desc) override final;
    Ref<Sampler>        CreateSampler(const SamplerDesc& desc) override final;
    void                SetName(const Ref<GpuResource>& resouce, const char* name) override final;

    /*** COMMAND LIST ***/
    CommandList*        BeginCommandList(QueueType type = QueueType::QUEUE_TYPE_GRAPHICS) override final;
    void                SubmitCommandList(CommandList* cmd, CommandList* waitedCmds = nullptr, uint32_t waitedCmdCounts = 0, bool signal = false) override final;
    
    /*** SWAPCHAIN ***/
    Image*              GetPresentImage() override final { return m_wsi.swapchain_images[m_wsi.swapchain_image_index].get(); }
    DataFormat          GetPresentImageFormat() override final;

    /*** PROPERTIES ***/
    bool                isFormatSupported(DataFormat format) override final;


    ///////////////////// Vulkan specific ////////////////////////
    //////////////////////////////////////////////////////////////
    DescriptorSetAllocator*     RequestDescriptorSetAllocator(const DescriptorSetLayout& layout);
    PipeLineLayout*             RequestPipeLineLayout(const ShaderResourceLayout& combinedLayout);
    PerFrameContext&               GetCurrentFrame();
    uint32_t 				    AllocateCookie(); 
    const VulkanContext&        GetVulkanContext() { return *m_vulkan_context.get(); }

    void DestroyBufferNoLock(VkBuffer buffer, VmaAllocation alloc);
    void DestroyBuffer(VkBuffer buffer, VmaAllocation alloc);
    void DestroyImageNoLock(VkImage image, VmaAllocation alloc);
    void DestroyImage(VkImage image, VmaAllocation alloc);
    void DestroyImageViewNoLock(VkImageView view);
    void DestroyImageView(VkImageView view);
    void RequestUniformBlock(BufferBlock& block, VkDeviceSize size);
    void RequestUniformBlockNoLock(BufferBlock& block, VkDeviceSize size);

private:
    void ResizeSwapchain();

    // represent a physical queue
    // responsible for queuing commad buffers and submit them in batch
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
    } m_queues[QUEUE_TYPE_MAX_ENUM];


    struct WindowSystemIntergration
    {
        Device_Vulkan* device = nullptr;
        std::vector<VkSemaphore> acquire_semaphores;
		std::vector<VkSemaphore> release_semaphores;
        std::vector<Ref<Image>> swapchain_images;
        uint8_t semaphore_index = 0;
        uint32_t swapchain_image_index;
        bool consumed = false;
        bool recreate_swapchain = false;

        void init(Device_Vulkan* device);
		void destroy();
    } m_wsi;

    struct
    {
        std::mutex memory_lock;
        std::mutex lock;
    } m_lock;

    Scope<VulkanContext> m_vulkan_context;
    std::vector<PerFrameContext> m_frames;
    uint8_t m_frame_index = 0;

    std::atomic_uint64_t m_cookie;

    // buffer pool
    BufferPool m_ubo_pool;

};
}