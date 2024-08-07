#include "TestBed/TestBed.h"
#include "Scene/Components/MoveControlCmpt.h"
#include <glm/gtx/quaternion.hpp>
#include <Quark/Asset/GLTFLoader.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Core/Window.h>
#include <Quark/UI/UI.h>
#include <imgui.h>

TestBed::TestBed(const AppInitSpecs& specs)
    : Application(specs)
{
    SetUpRenderPass();
    CreatePipeline();
    CreateDepthImage();

    LoadScene();
}

Application* CreateApplication()
{
    AppInitSpecs specs = {
        .uiSpecs = {
            .flags = 0
        },
        .title = "TestBed",
        .width = 1200,
        .height = 800,
        .isFullScreen = false
    };

    return new TestBed(specs);
    
}

TestBed::~TestBed()
{
}

void TestBed::Update(f32 deltaTime)
{    
    // Update camera movement
    auto* cam = scene->GetCamera();
    auto* camMoveCmpt = cam->GetEntity()->GetComponent<MoveControlCmpt>();
    camMoveCmpt->Update(deltaTime);

    // Update scene
    scene->Update();

    // Update UI
    UpdateUI();
}   

void TestBed::UpdateUI()
{
    // Prepare UI data
    UI::Singleton()->BeginFrame();
    
    if (ImGui::Begin("Background")) {
    
        ImGui::Text("FPS: %f", m_Status.fps);
        ImGui::Text("Frame Time: %f ms", m_Status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", cmdListRecordTime);
    }
    ImGui::End();

    UI::Singleton()->EndFrame();
}

void TestBed::SetUpRenderPass()
{   
    // First pass : geometry pass
    geometry_pass_info = {};
    geometry_pass_info.numColorAttachments = 1;
    geometry_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    geometry_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    geometry_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetSwapChainImageFormat();
    geometry_pass_info.depthAttachment = depth_image.get();
    geometry_pass_info.depthAttachmentLoadOp = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    geometry_pass_info.depthAttachmentStoreOp = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    geometry_pass_info.ClearDepthStencil.depth_stencil = {1.f, 0};
    geometry_pass_info.depthAttachmentFormat = depth_format;

    // Second pass : UI pass
    ui_pass_info = {};
    ui_pass_info.numColorAttachments = 1;
    ui_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::LOAD;
    ui_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    ui_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetSwapChainImageFormat();

}

void TestBed::Render(f32 deltaTime)
{
    // Fill command list
    auto graphic_device = Application::Instance().GetGraphicDevice();
    if (graphic_device->BeiginFrame(deltaTime)) {
        // 1. Begin a graphic command list
        auto cmd = graphic_device->BeginCommandList();

        // 2. Query swapchain image and add layout transition barrier
        auto swap_chain_image = graphic_device->GetPresentImage();
        
        graphic::PipelineImageBarrier swapchain_image_barrier{
            .image = swap_chain_image,
            .srcStageBits = graphic::PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            .srcMemoryAccessBits = 0,
            .dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .layoutBefore = graphic::ImageLayout::UNDEFINED,
            .layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
        };
        cmd->PipeLineBarriers(nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

        // 3. Geometry pass
        geometry_pass_info.colorAttachments[0] = swap_chain_image;
        geometry_pass_info.clearColors[0] = {0.f, 0.f, 0.3f, 1.f};
        geometry_pass_info.depthAttachment = depth_image.get();
        cmd->BeginRenderPass(geometry_pass_info);

        // Bind Pipeline, set viewport and scissor
        cmd->BindPipeLine(*graphic_pipeline);

        cmd->SetViewPort(graphic::Viewport{.x = 0, .y = 0, .width = (float)swap_chain_image->GetDesc().width,
            .height = (float)swap_chain_image->GetDesc().height, .minDepth = 0, .maxDepth = 1});

        cmd->SetScissor(graphic::Scissor{.extent = {.width = swap_chain_image->GetDesc().width, .height = swap_chain_image->GetDesc().height},
            .offset = {.x = 0, .y = 0}});
        
        // Draw scene
        auto geometry_start = m_Timer.ElapsedMillis();
        scene_renderer->RenderScene(cmd);
        cmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

        cmd->EndRenderPass();

        // 4. Beigin UI pass
        ui_pass_info.colorAttachments[0] = swap_chain_image;
        cmd->BeginRenderPass(ui_pass_info);
        UI::Singleton()->Render(cmd);
        cmd->EndRenderPass();

        // 7. Transit swapchain image to present layout for presenting
        graphic::PipelineImageBarrier present_barrier {
            .image = swap_chain_image,
            .srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstMemoryAccessBits = 0,
            .layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            .layoutAfter = graphic::ImageLayout::PRESENT
        };
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
    asset::GLTFLoader gltf_loader(m_GraphicDevice.get());
    scene = gltf_loader.LoadSceneFromFile("Assets/Gltf/house2.glb");
    
    // Create camera node
    float aspect = (float)Window::Instance()->GetWidth() / Window::Instance()->GetHeight();
    auto* cam_node = scene->CreateNode("Main camera", nullptr);
    cam_node->GetEntity()->AddComponent<CameraCmpt>(aspect, 60.f, 0.1f, 256);
    cam_node->GetEntity()->AddComponent<MoveControlCmpt>(50, 0.3);

    // Default position
    auto* transform_cmpt = cam_node->GetEntity()->GetComponent<TransformCmpt>();
    transform_cmpt->SetPosition(glm::vec3(0, 0, 10));

    scene->SetCamera(cam_node);
    scene_renderer = CreateScope<render::SceneRenderer>(m_GraphicDevice.get());
    scene_renderer->SetScene(scene.get());
}

void TestBed::CreatePipeline()
{
    using namespace quark::graphic;
    auto graphic_device = Application::Instance().GetGraphicDevice();

    // Create shader
    vert_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX, "Assets/Shaders/Spirv/pbr.vert.spv");
    frag_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT, "Assets/Shaders/Spirv/pbr.frag.spv");
    
    // Create pipeline
    GraphicPipeLineDesc pipe_desc;
    pipe_desc.vertShader = vert_shader;
    pipe_desc.fragShader = frag_shader;
    pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
    pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
    pipe_desc.renderPassInfo = geometry_pass_info;
    pipe_desc.depthStencilState = {
        .enableDepthTest = true,
        .enableDepthWrite = true,
        .depthCompareOp = CompareOperation::LESS_OR_EQUAL
    };
    pipe_desc.rasterState = {
        .cullMode = CullMode::NONE,
        .polygonMode = PolygonMode::Fill,
        .frontFaceType = FrontFaceType::COUNTER_CLOCKWISE
    };

    graphic_pipeline = graphic_device->CreateGraphicPipeLine(pipe_desc);
}

void TestBed::CreateDepthImage()
{
        using namespace quark::graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // Image create info
        ImageDesc depth_image_desc = {
            .type = ImageType::TYPE_2D,
            .width = graphic_device->GetResolutionWidth(),
            .height = graphic_device->GetResolutionHeight(),
            .depth = 1,
            .format = depth_format,
            .arraySize = 1,
            .mipLevels = 1,
            .initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        };

        // Create depth image
        depth_image = graphic_device->CreateImage(depth_image_desc);
}
