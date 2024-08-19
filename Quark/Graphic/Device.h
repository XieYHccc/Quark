#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/TimeStep.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Graphic/Buffer.h"
#include "Quark/Graphic/Image.h"
#include "Quark/Graphic/CommandList.h"
#include "Quark/Graphic/Shader.h"
#include "Quark/Graphic/PipeLine.h"
#include "Quark/Graphic/RenderPassInfo.h"

namespace quark::graphic {

struct DeviceProperties {
    struct DeviceLimits {
        u64 minUniformBufferOffsetAlignment = 0;
    } limits;

};
struct DeviceFeatures {
    bool textureCompressionBC = false;
    bool textureCompressionASTC_LDR = false;
    bool textureCompressionETC2 = false;
};

class Device {
public:
    u32 currentFrame;
    u32 frameBufferWidth;
    u32 frameBufferHeight;
    DeviceProperties properties;
    DeviceFeatures features;
    
public:
    Device() = default;
    virtual ~Device() = default;

    u32 GetResolutionWidth() { return frameBufferWidth;}
    u32 GetResolutionHeight() { return frameBufferHeight;}
    const DeviceProperties& GetDeviceProperties() const { return properties; }
    
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

	/*** COMMANDS ***/
    virtual CommandList* BeginCommandList(QueueType type = QueueType::QUEUE_TYPE_GRAPHICS) = 0;
    virtual void SubmitCommandList(CommandList* cmd, CommandList* waitedCmds = nullptr, uint32_t waitedCmdCounts = 0, bool signal = false) = 0;

	/*** SWAPCHAIN ***/
    virtual Image* GetPresentImage() = 0; // Owned by device.
    virtual DataFormat GetSwapChainImageFormat() = 0;

    /*** PROPERTIES ***/
    virtual bool isFormatSupported(DataFormat format) = 0;
};

}