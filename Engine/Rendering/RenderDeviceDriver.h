#pragma once

#include "Util/Singleton.h"
#include "Rendering/RenderTypes.h"

class RenderDeviceDriver {
public:
    u32 currentFrame;
    f64 frameNumber;

public:
    RenderDeviceDriver();
    virtual ~RenderDeviceDriver();

    virtual bool Init() = 0;
    virtual void ShutDown() = 0;
    virtual bool BeiginFrame(f32 deltaTime) = 0;
    virtual bool EndFrame(f32 deltaTime) = 0;
    virtual void Resize(u16 width, u16 height) = 0;

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
	/*** Rendering ***/
	/*****************/
    // virtual void RenderPassCreate() = 0;
    // virtual void RenderPassFree() = 0;
};
