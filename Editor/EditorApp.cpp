#include "Editor/EditorApp.h"
#include <glm/gtx/quaternion.hpp>
#include <Quark/Asset/GLTFLoader.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Core/Window.h>
#include <Quark/UI/UI.h>
#include <Quark/Scene/Components/MoveControlCmpt.h>
#include <imgui.h>

#include "Editor/CameraControlCmpt.h"

Application* CreateApplication()
{    
    AppInitSpecs specs = {
        .uiInitFlags = UI_INIT_FLAG_DOCKING | UI_INIT_FLAG_VIEWPORTS,
        .title = "Quark Editor",
        .width = 1300,
        .height = 800,
        .isFullScreen = false
    };

    return new editor::EditorApp(specs);
}

namespace editor {

EditorApp::EditorApp(const AppInitSpecs& specs)
    : Application(specs)
{
    color_format = m_GraphicDevice->GetSwapChainImageFormat();

    // Render structures
    SetUpRenderPass();
    CreatePipeline();
    CreateColorDepthAttachments();

    // Load scene
    LoadScene();

    // Init UI windows
    heirarchyWindow_.Init();
    heirarchyWindow_.SetScene(scene_.get());
    inspector_.Init();
    sceneViewPort_.Init();;
}

EditorApp::~EditorApp()
{
}

void EditorApp::Update(f32 deltaTime)
{    
    // Update camera movement
    auto* editorCameraCmpt = scene_->GetCamera();
    auto* cameraMoveCmpt = editorCameraCmpt->GetEntity()->GetComponent<component::EditorCameraControlCmpt>();
    cameraMoveCmpt->Update(deltaTime);

    // Update scene
    scene_->Update();

    // Update UI
    UpdateUI();
}   

void EditorApp::UpdateUI()
{
    // Prepare UI data
    UI::Singleton()->BeginFrame();

    // Debug Ui
    if (ImGui::Begin("Debug")) {
    
        ImGui::Text("FPS: %f", m_Status.fps);
        ImGui::Text("Frame Time: %f ms", m_Status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", cmdListRecordTime);
    }
    ImGui::End();

    // Update Scene Heirarchy
    heirarchyWindow_.Render();
    
    // Update Inspector
    inspector_.SetNode(heirarchyWindow_.GetSelectedNode());
    inspector_.Render();

    // Update Scene view port
    sceneViewPort_.SetColorAttachment(color_image.get());
    sceneViewPort_.Render();
    
    // Update Main menu bar
    UpdateMainMenuUI();

    UI::Singleton()->EndFrame();
}

void EditorApp::Render(f32 deltaTime)
{
    auto graphic_device = Application::Instance().GetGraphicDevice();

    if (graphic_device->BeiginFrame(deltaTime)) {
        auto cmd = graphic_device->BeginCommandList();
        auto* swap_chain_image = graphic_device->GetPresentImage();

        // Geometry pass
        {
            graphic::PipelineImageBarrier image_barrier{
                .image = color_image.get(),
                .srcStageBits = graphic::PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                .srcMemoryAccessBits = 0,
                .dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .layoutBefore = graphic::ImageLayout::UNDEFINED,
                .layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
            };

            cmd->PipeLineBarriers(nullptr, 0, &image_barrier, 1, nullptr, 0);

            geometry_pass_info.colorAttachments[0] = color_image.get();
            geometry_pass_info.clearColors[0] = {0.5f, 0.5f, 0.5f, 1.f};
            geometry_pass_info.depthAttachment = depth_image.get();
            cmd->BeginRenderPass(geometry_pass_info);

            // Bind Pipeline, set viewport and scissor
            cmd->BindPipeLine(*graphic_pipeline);

            cmd->SetViewPort(graphic::Viewport{.x = 0, .y = 0, .width = (float)color_image->GetDesc().width,
                .height = (float)color_image->GetDesc().height, .minDepth = 0, .maxDepth = 1});

            cmd->SetScissor(graphic::Scissor{.extent = {.width = color_image->GetDesc().width, .height = color_image->GetDesc().height},
                .offset = {.x = 0, .y = 0}});
            
            // Draw scene
            auto geometry_start = m_Timer.ElapsedMillis();
            scene_renderer_->Render(cmd);
            cmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

            cmd->EndRenderPass();
        }

        // UI pass
        {
            graphic::PipelineImageBarrier swapchian_image_barrier{
                .image = swap_chain_image,
                .srcStageBits = graphic::PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .srcMemoryAccessBits = 0,
                .dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .layoutBefore = graphic::ImageLayout::UNDEFINED,
                .layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
            };

            graphic::PipelineImageBarrier color_image_barrier{
                .image = color_image.get(),
                .srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageBits = graphic::PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstMemoryAccessBits = graphic::BARRIER_ACCESS_SHADER_READ_BIT,
                .layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                .layoutAfter = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL
            };

            cmd->PipeLineBarriers(nullptr, 0, &swapchian_image_barrier, 1, nullptr, 0);
            cmd->PipeLineBarriers(nullptr, 0, &color_image_barrier, 1, nullptr, 0);

            ui_pass_info.colorAttachments[0] = swap_chain_image;
            cmd->BeginRenderPass(ui_pass_info);
            UI::Singleton()->Render(cmd);
            cmd->EndRenderPass();
        }


        // Transit swapchain image to present layout for presenting
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

        // Submit command list
        graphic_device->SubmitCommandList(cmd);
    
        graphic_device->EndFrame(deltaTime);
    }
}

void EditorApp::LoadScene()
{
    // load scene
    asset::GLTFLoader gltf_loader(m_GraphicDevice.get());
    scene_ = gltf_loader.LoadSceneFromFile("/Users/xieyhccc/develop/Quark/Assets/Gltf/teapot.gltf");

    // Create camera node
    float aspect = (float)Window::Instance()->GetWidth() / Window::Instance()->GetHeight();
    auto* cam_node = scene_->CreateNode("Editor Camera", nullptr);
    cam_node->GetEntity()->AddComponent<scene::CameraCmpt>(aspect, 60.f, 0.1f, 256);
    cam_node->GetEntity()->AddComponent<component::EditorCameraControlCmpt>(50, 0.3);

    // Default position
    auto* transform_cmpt = cam_node->GetEntity()->GetComponent<scene::TransformCmpt>();
    transform_cmpt->SetPosition(glm::vec3(0, 0, 10));

    scene_->SetCamera(cam_node);

    // Set scene to renderer
    scene_renderer_ = CreateScope<render::SceneRenderer>(m_GraphicDevice.get());
    scene_renderer_->SetScene(scene_.get());
}

void EditorApp::UpdateMainMenuUI()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load Project")) {
                // LoadProject();
			}
			ImGui::EndMenu();
		}

        ImGui::EndMainMenuBar();
    }
}

void EditorApp::CreatePipeline()
{
    using namespace graphic;
    auto graphic_device = Application::Instance().GetGraphicDevice();

    // Create shader
    vert_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_VERTEX,
        "/Users/xieyhccc/develop/Quark/Assets/Shaders/Spirv/pbr.vert.spv");
    frag_shader = graphic_device->CreateShaderFromSpvFile(ShaderStage::STAGE_FRAGEMNT,
        "/Users/xieyhccc/develop/Quark/Assets/Shaders/Spirv/pbr.frag.spv");
    
    // Create pipeline
    GraphicPipeLineDesc pipe_desc;
    pipe_desc.vertShader = vert_shader;
    pipe_desc.fragShader = frag_shader;
    pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
    pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
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

    graphic_pipeline = graphic_device->CreateGraphicPipeLine(pipe_desc, geometry_pass_info);
}

void EditorApp::CreateColorDepthAttachments()
{
        using namespace graphic;
        auto graphic_device = Application::Instance().GetGraphicDevice();

        // Image create info
        ImageDesc image_desc = {
            .type = ImageType::TYPE_2D,
            .width = u32(Window::Instance()->GetMonitorWidth() * Window::Instance()->GetRatio()),
            .height = u32(Window::Instance()->GetMonitorHeight() * Window::Instance()->GetRatio()),
            .depth = 1,
            .format = depth_format,
            .arraySize = 1,
            .mipLevels = 1,
            .initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        };

        // Create depth image
        depth_image = graphic_device->CreateImage(image_desc);

        // Create color image
        image_desc.format = color_format;
        image_desc.initialLayout = ImageLayout::UNDEFINED;
        image_desc.usageBits = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | graphic::IMAGE_USAGE_SAMPLING_BIT;
        color_image = graphic_device->CreateImage(image_desc);
}


void EditorApp::SetUpRenderPass()
{   
    // First pass : geometry pass
    geometry_pass_info = {};
    geometry_pass_info.numColorAttachments = 1;
    geometry_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    geometry_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    geometry_pass_info.colorAttachmentFormats[0] = color_format;
    geometry_pass_info.useDepthAttachment = true;
    geometry_pass_info.depthAttachment = depth_image.get();
    geometry_pass_info.depthAttachmentLoadOp = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    geometry_pass_info.depthAttachmentStoreOp = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    geometry_pass_info.ClearDepthStencil.depth_stencil = {1.f, 0};
    geometry_pass_info.depthAttachmentFormat = depth_format;

    // Second pass : UI pass
    ui_pass_info = {};
    ui_pass_info.numColorAttachments = 1;
    ui_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    ui_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    ui_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetSwapChainImageFormat();

}
}