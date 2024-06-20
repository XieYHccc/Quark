#pragma once
#include "Core/Base.h"
#include "Events/ApplicationEvent.h"
#include "Graphic/Common.h"
#include "Graphic/Buffer.h"
#include "Graphic/Image.h"
#include "Graphic/CommandList.h"
#include "Graphic/Shader.h"
#include "Graphic/PipeLine.h"
#include "Graphic/RenderPassInfo.h"

namespace graphic {
class Device {
public:
    u32 currentFrame;
    u32 frameBufferWidth;
    u32 frameBufferHeight;

public:
    Device() = default;
    virtual ~Device() = default;

    virtual bool Init() = 0;
    virtual void ShutDown() = 0;
    virtual bool BeiginFrame(f32 deltaTime) = 0;
    virtual bool EndFrame(f32 deltaTime) = 0;
    virtual void OnWindowResize(const WindowResizeEvent& event) = 0;    // window resize callback

	/*** RESOURCES ***/  
public:
    virtual Ref<Buffer> CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual Ref<Image> CreateImage(const ImageDesc& desc, const ImageInitData* initdata) = 0;
    virtual Ref<Shader> CreateShaderFromBytes(ShaderStage stage, const void* byteCode, size_t codeSize) = 0;
    virtual Ref<Shader> CreateShaderFromSpvFile(ShaderStage stage, const std::string& file_path) = 0;
    virtual Ref<PipeLine> CreateGraphicPipeLine(const GraphicPipeLineDesc& desc) = 0;
    
	/*** COMMANDS ***/
    virtual CommandList* BeginCommandList(QueueType type = QueueType::QUEUE_TYPE_GRAPHICS) = 0;
    virtual void SubmitCommandList(CommandList* cmd, CommandList* waitedCmds = nullptr, uint32_t waitedCmdCounts = 0, bool signal = false) = 0;

	/*** SWAPCHAIN ***/
    virtual Image* GetSwapChainImage() = 0; // Owned by device.
    virtual DataFormat GetSwapChainImageFormat() = 0;
};

}