#pragma once
#include "Quark/Core/Util/Hash.h"
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/RHI/Common.h"

namespace quark::rhi {
union ClearValue
{
    // float color[4];
    union clearColorValue
	{
		float float32[4];
        uint32_t uint32[4];
        int32_t int32[4];
	} color;

    struct ClearDepthStencilValue
    {
        float depth;
        uint32_t stencil;
    } depth_stencil;
};

// This is all information about renderpass that need to be known when creating a pipeline
struct RenderPassInfo
{
    DataFormat colorAttachmentFormats[MAX_COLOR_ATTHACHEMNT_NUM] = {};
    uint32_t numColorAttachments = 0;

    DataFormat depthAttachmentFormat = DataFormat::UNDEFINED;
    SampleCount sampleCount = SampleCount::SAMPLES_1;

    bool IsValid() const { return numColorAttachments > 0; }

    uint64_t GetHash() const
    {
        util::Hasher h;
        for (size_t i = 0; i < numColorAttachments; i++)
            h.u32(util::ecast(colorAttachmentFormats[i]));

        h.u32(util::ecast(depthAttachmentFormat));

        return h.get();
    }
};

struct FrameBufferInfo
{
    enum class AttachmentLoadOp
    {
        LOAD,
        CLEAR,
        DONTCARE,
    };

    enum class AttachmentStoreOp
    {
        STORE,
        DONTCARE,
    };

	ImageView* colorAttachments[MAX_COLOR_ATTHACHEMNT_NUM]; // the number should be equal to the number of color attachments in renderpass
    ClearValue clearColors[MAX_COLOR_ATTHACHEMNT_NUM] = {};
    AttachmentLoadOp colorAttatchemtsLoadOp[MAX_COLOR_ATTHACHEMNT_NUM];
    AttachmentStoreOp colorAttatchemtsStoreOp[MAX_COLOR_ATTHACHEMNT_NUM];

    // Resolve image. The declaration of color 
    // attachments and resolve attachments must match
    // to deduce source color image and the destination resolve image.
    ImageView* resolveAttatchments[MAX_COLOR_ATTHACHEMNT_NUM];
    u32 numResolveAttachments = 0;

    ImageView* depthAttachment = nullptr;
    AttachmentLoadOp depthAttachmentLoadOp = AttachmentLoadOp::CLEAR;
    AttachmentStoreOp depthAttachmentStoreOp = AttachmentStoreOp::STORE;
    ClearValue clearDepthStencil = { 1.f, 0.f };

    ImageView* depthResolveAttachment = nullptr;
};

}