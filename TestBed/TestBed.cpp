#include "TestBed/TestBed.h"
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <Quark/Core/Window.h>
#include <Quark/Asset/GLTFImporter.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Scene/Components/MoveControlCmpt.h>
#include <Quark/Renderer/Renderer.h>
#include <Quark/UI/UI.h>

namespace quark {

TestBed::TestBed(const ApplicationSpecification& specs)
    : Application(specs)
{
    CreateRenderPassInfos();
    CreateDepthImage();

    LoadScene();

}

Application* CreateApplication()
{
    ApplicationSpecification specs;
    specs.height = 800;
    specs.width = 1200;
    specs.isFullScreen = false;
    specs.title = "TestBed";
    specs.uiSpecs.flags = 0;

    return new TestBed(specs);

}

TestBed::~TestBed()
{

}

void TestBed::OnUpdate(TimeStep deltaTime)
{
    // Update camera movement
    auto* camEntity = scene->GetMainCameraEntity();
    auto* camMoveCmpt = camEntity->GetComponent<MoveControlCmpt>();
    camMoveCmpt->Update(deltaTime);

    // Update scene
    scene->OnUpdate();
}

void TestBed::OnImGuiUpdate()
{
    // Prepare UI data
    UI::Get()->BeginFrame();

    if (ImGui::Begin("Background")) {

        ImGui::Text("FPS: %f", m_Status.fps);
        ImGui::Text("Frame Time: %f ms", m_Status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", cmdListRecordTime);
    }
    ImGui::End();

    UI::Get()->EndFrame();
}

void TestBed::CreateRenderPassInfos()
{
    // First pass : geometry pass
    geometry_pass_info = {};
    geometry_pass_info.numColorAttachments = 1;

    geometry_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetPresentImageFormat();
    geometry_pass_info.depthAttachmentFormat = depth_format;

    // Second pass : UI pass
    ui_pass_info = {};
    ui_pass_info.numColorAttachments = 1;
    ui_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetPresentImageFormat();

}

void TestBed::OnRender(TimeStep deltaTime)
{
    // Fill command list
    auto graphic_device = Application::Get().GetGraphicDevice();

    if (graphic_device->BeiginFrame(deltaTime)) {
        // 1. Begin a graphic command list
        auto cmd = graphic_device->BeginCommandList();

        // 2. Query swapchain image and add layout transition barrier
        auto swap_chain_image = graphic_device->GetPresentImage();

        graphic::PipelineImageBarrier swapchain_image_barrier;
        swapchain_image_barrier.image = swap_chain_image;
        swapchain_image_barrier.srcStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        swapchain_image_barrier.srcMemoryAccessBits = 0;
        swapchain_image_barrier.dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        swapchain_image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        swapchain_image_barrier.layoutBefore = graphic::ImageLayout::UNDEFINED;
        swapchain_image_barrier.layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        cmd->PipeLineBarriers(nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

        // 3. Geometry pass
        graphic::FrameBufferInfo fb_info;
        fb_info.colorAttachments[0] = swap_chain_image;
        fb_info.depthAttachment = depth_image.get();
        fb_info.colorAttatchemtsLoadOp[0] = graphic::FrameBufferInfo::AttachmentLoadOp::CLEAR;
        fb_info.colorAttatchemtsStoreOp[0] = graphic::FrameBufferInfo::AttachmentStoreOp::STORE;
        fb_info.depthAttachmentLoadOp = graphic::FrameBufferInfo::AttachmentLoadOp::CLEAR;
        fb_info.depthAttachmentStoreOp = graphic::FrameBufferInfo::AttachmentStoreOp::STORE;
        fb_info.clearColors[0] = { 0.22f, 0.22f, 0.22f, 1.f };
        fb_info.clearDepthStencil.depth_stencil = { 1.f, 0 };
        cmd->BeginRenderPass(geometry_pass_info, fb_info);

        graphic::Viewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)swap_chain_image->GetDesc().width;
        viewport.height = (float)swap_chain_image->GetDesc().height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;

        graphic::Scissor scissor;
        scissor.extent.width = swap_chain_image->GetDesc().width;
        scissor.extent.height = swap_chain_image->GetDesc().height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        cmd->SetViewPort(viewport);
        cmd->SetScissor(scissor);

        // Draw scene
        auto geometry_start = m_Timer.ElapsedMillis();
        scene_renderer->UpdateDrawContext();
        scene_renderer->DrawScene(cmd);
        cmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

        cmd->EndRenderPass();

        // 4. Beigin UI pass
        fb_info.colorAttatchemtsLoadOp[0] = graphic::FrameBufferInfo::AttachmentLoadOp::LOAD;
        fb_info.colorAttatchemtsStoreOp[0] = graphic::FrameBufferInfo::AttachmentStoreOp::STORE;
        fb_info.depthAttachment = nullptr;
        cmd->BeginRenderPass(ui_pass_info, fb_info);
        UI::Get()->OnRender(cmd);
        cmd->EndRenderPass();

        // 7. Transit swapchain image to present layout for presenting
        graphic::PipelineImageBarrier present_barrier;
        present_barrier.image = swap_chain_image;
        present_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        present_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        present_barrier.dstStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        present_barrier.dstMemoryAccessBits = 0;
        present_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        present_barrier.layoutAfter = graphic::ImageLayout::PRESENT;
        cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);

        // 8. Submit command list
        graphic_device->SubmitCommandList(cmd);

        // End this frame, submit Command list and pressent to screen
        graphic_device->EndFrame(deltaTime);
    }

}

void TestBed::LoadScene()
{
    // load scene
    GLTFImporter gltf_loader;
    gltf_loader.Import("BuiltInResources/Gltf/house2.glb");

    scene = gltf_loader.GetScene();

    // Create camera node
    float aspect = GetWindow()->GetRatio();
    auto* camEntity = scene->CreateEntity("Main camera Entity", nullptr);
    camEntity->AddComponent<CameraCmpt>(aspect, 60.f, 0.1f, 256);
    camEntity->AddComponent<MoveControlCmpt>(50, 0.3);

    // Default position
    auto* transform_cmpt = camEntity->GetComponent<TransformCmpt>();
    transform_cmpt->SetLocalPosition(glm::vec3(0, 0, 10));

    scene->SetMainCameraEntity(camEntity);
    scene_renderer = CreateScope<SceneRenderer>(m_GraphicDevice.get());
    scene_renderer->SetScene(scene);
}

void TestBed::CreateDepthImage()
{
    using namespace quark::graphic;

    // Image create info
    ImageDesc depth_image_desc;
    depth_image_desc.type = ImageType::TYPE_2D;
    depth_image_desc.width = m_GraphicDevice->GetResolutionWidth();
    depth_image_desc.height = m_GraphicDevice->GetResolutionHeight();
    depth_image_desc.depth = 1;
    depth_image_desc.format = depth_format;
    depth_image_desc.arraySize = 1;
    depth_image_desc.mipLevels = 1;
    depth_image_desc.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_image_desc.usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    depth_image = m_GraphicDevice->CreateImage(depth_image_desc);
}

}