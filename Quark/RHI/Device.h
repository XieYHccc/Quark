#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/TimeStep.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/RHI/Common.h"
#include "Quark/RHI/Buffer.h"
#include "Quark/RHI/Image.h"
#include "Quark/RHI/CommandList.h"
#include "Quark/RHI/Shader.h"
#include "Quark/RHI/PipeLine.h"
#include "Quark/RHI/RenderPassInfo.h"

namespace quark::rhi {

struct DeviceProperties 
{
    struct DeviceLimits 
    {
        u64 minUniformBufferOffsetAlignment = 0;
    } limits;

};

struct DeviceFeatures 
{
    bool textureCompressionBC = false;
    bool textureCompressionASTC_LDR = false;
    bool textureCompressionETC2 = false;
};

class Device 
{
public:
    Device() = default;
    virtual ~Device() = default;

    uint32_t GetResolutionWidth() { return m_frameBufferWidth;}
    uint32_t GetResolutionHeight() { return m_frameBufferHeight;}
    uint32_t GetCurrentFrameIndex() { return m_elapsedFrame % MAX_FRAME_NUM_IN_FLIGHT; }
    uint32_t GetFrameCount() { return MAX_FRAME_NUM_IN_FLIGHT; }

    const DeviceProperties& GetDeviceProperties() const { return m_properties; }
    const DeviceFeatures& GetDeviceFeatures() const { return m_features; }
    
    virtual bool Init() = 0;
    virtual void ShutDown() = 0;
    virtual bool BeiginFrame(TimeStep ts) = 0;
    virtual bool EndFrame(TimeStep ts) = 0;
    virtual void OnWindowResize(const WindowResizeEvent& event) = 0;    // window resize callback

	/*** RESOURCES ***/  
    virtual Ref<Buffer> CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual Ref<Image> CreateImage(const ImageDesc& desc, const ImageInitData* initdata = nullptr) = 0;
    virtual Ref<Shader> CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize) = 0;
    virtual Ref<Shader> CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path) = 0;
    virtual Ref<PipeLine> CreateGraphicPipeLine(const GraphicPipeLineDesc& desc) = 0;
    virtual Ref<Sampler> CreateSampler(const SamplerDesc& desc) = 0;

	/*** COMMAND LIST ***/
    virtual CommandList* BeginCommandList(QueueType type = QueueType::QUEUE_TYPE_GRAPHICS) = 0;
    virtual void SubmitCommandList(CommandList* cmd, CommandList* waitedCmds = nullptr, uint32_t waitedCmdCounts = 0, bool signal = false) = 0;

	/*** SWAPCHAIN ***/
    virtual Image* GetPresentImage() = 0; // Owned by device.
    virtual DataFormat GetPresentImageFormat() = 0;

    /*** PROPERTIES ***/
    virtual bool isFormatSupported(DataFormat format) = 0;
    virtual void SetDebugName(const Ref<GpuResource>& resouce, const char* name) = 0;

protected:
    uint32_t m_elapsedFrame = 0;
    uint32_t m_frameBufferWidth;
    uint32_t m_frameBufferHeight;
    DeviceProperties m_properties;
    DeviceFeatures m_features;
};

}