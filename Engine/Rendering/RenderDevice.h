#pragma once
#include "Util/Singleton.h"
#include "Events/ApplicationEvent.h"
#include "Rendering/RenderTypes.h"


enum QueueType
{
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_ASYNC_COMPUTE,
    QUEUE_TYPE_ASYNC_TRANSFER,
    QUEUE_TYPE_MAX_ENUM,
};

struct CommandList {
    void* internal = nullptr;
    bool isValid() { return internal;}
};

class RenderDevice : public MakeSingletonPtr<RenderDevice>{
public:
    u32 currentFrame;
    u32 frameBufferWidth;
    u32 frameBufferHeight;

public:
    RenderDevice();
    virtual ~RenderDevice();

    virtual bool Init() = 0;
    virtual void ShutDown() = 0;
    virtual bool BeiginFrame(f32 deltaTime) = 0;
    virtual bool EndFrame(f32 deltaTime) = 0;

    // Evnets callbacks
    virtual void OnWindowResize(const WindowResizeEvent& event) = 0;

	/*****************/
	/**** BUFFERS ****/
	/*****************/
	// virtual void BufferCreate() = 0;
	// virtual void BufferFree() = 0;

    /*****************/
	/**** TEXTURE ****/
	/*****************/
    virtual void ImageCreate(const GPUImageDesc& desc, GPUImage* outImage) = 0;
    virtual void ImageFree(GPUImage* image) = 0;

    /*****************/
	/***** SHADER ****/
	/*****************/

    virtual void ShaderCreateFromBytes(ShaderStage stage, void* byteCode, size_t byteCount, Shader* outShader) = 0;
    virtual bool ShaderCreateFromSpvFile(ShaderStage stage, const std::string& name, Shader* outShader) = 0;
    virtual void ShaderFree(Shader* shader) = 0;

    /******************************/
	/**** PIPELINE STATE OBJECT ***/
	/******************************/
    virtual bool GraphicPipeLineCreate(PipeLineDesc& desc, PipeLine* outPSO) = 0;
    virtual bool ComputePipeLineCreate(Shader* compShader, PipeLine* outPSO) = 0;
    virtual void PipeLineFree(PipeLine* pipeline) = 0;

    /********************/
	/*** Command List ***/
	/********************/
    virtual CommandList CommandListBegin(QueueType type = QUEUE_TYPE_GRAPHICS) = 0;
    virtual void CommandListEnd(CommandList cmd) = 0;
    // virtual void CommandListWait (CommandList cmd, CommandList waitCmd)  = 0;

    /*****************/
	/*** Rendering ***/
	/*****************/
    virtual void CmdBeginRenderPass(CommandList cmd, RenderPass& renderPass) = 0;
    virtual void CmdEndRenderPass(CommandList cmd) = 0;
    virtual void CmdPipeLineBarriers(CommandList cmd, PipelineMemoryBarrier* memoryBarriers, u32 memoryBarriersCount, PipelineImageBarrier* iamgeBarriers, u32 iamgeBarriersCount, PipelineBufferBarrier* bufferBarriers, u32 bufferBarriersCount) = 0;

    /*****************/
	/*** SWAPCHAIN ***/
	/*****************/
    virtual GPUImage* GetSwapChainImage() = 0;
    
};

template<>
template<>
RenderDevice& MakeSingletonPtr<RenderDevice>::CreateSingleton();