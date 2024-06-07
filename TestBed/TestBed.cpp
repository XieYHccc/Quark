#include "TestBed/TestBed.h"
#include <Rendering/RenderDevice.h>

TestBed::TestBed(const std::string& title, const std::string& root, int width, int height)
    : Application(title, root, width, height)
{
    
}

Application* CreateApplication()
{
    return new TestBed("TestBed"," ", 1200, 800);
}

TestBed::~TestBed()
{

}

void TestBed::Update(f32 deltaTime)
{
}

void TestBed::Render(f32 deltaTime)
{
    if (RenderDevice::Singleton().BeiginFrame(deltaTime)) {
        // 1. Get graphics command list
        auto cmd = RenderDevice::Singleton().CommandListBegin();

        // 2. Query swapchain image and add layout transition barrier
        auto swap_chain_image = RenderDevice::Singleton().GetSwapChainImage();
        
        PipelineImageBarrier swapchain_image_barrier{
            .image = swap_chain_image,
            .srcStageBits = PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            .srcMemoryAccessBits = 0,
            .dstStageBits = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstMemoryAccessBits = BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .layoutBefore = GPUImageLayout::UNDEFINED,
            .layoutAfter = GPUImageLayout::COLOR_ATTACHMENT_OPTIMAL
        };

        RenderDevice::Singleton().CmdPipeLineBarriers(cmd, nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

        // 3. Begin a render pass
        RenderPass render_pass;
        render_pass.numColorAttachments = 1;
        render_pass.colorAttachments[0] = swap_chain_image;
        render_pass.colorAttatchemtsLoadOp[0] = RenderPass::AttachmentLoadOp::CLEAR;
        render_pass.colorAttatchemtsStoreOp[0] = RenderPass::AttachmentStoreOp::STORE;
        render_pass.clearColors[0].color[0] = 0.f;
        render_pass.clearColors[0].color[1] = 0.f;
        render_pass.clearColors[0].color[2] = 0.3f;
        render_pass.clearColors[0].color[3] = 0.f;
        RenderDevice::Singleton().CmdBeginRenderPass(cmd, render_pass);

        RenderDevice::Singleton().CmdEndRenderPass(cmd);

        // 4. Transit swapchain image to present layout for presenting
        PipelineImageBarrier present_barrier {
            .image = swap_chain_image,
            .srcStageBits = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcMemoryAccessBits = BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageBits = PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstMemoryAccessBits = 0,
            .layoutBefore = GPUImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            .layoutAfter = GPUImageLayout::PRESENT
        };
        RenderDevice::Singleton().CmdPipeLineBarriers(cmd, nullptr, 0, &present_barrier, 1, nullptr, 0);

        // 5. End this command list
        RenderDevice::Singleton().CommandListEnd(cmd);

        // End this frame and submit Command list and pressent to screen
        RenderDevice::Singleton().EndFrame(deltaTime);
    }
}