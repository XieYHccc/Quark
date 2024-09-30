#include "Editor/EditorApp.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

#include <Quark/Core/Application.h>
#include <Quark/Core/FileSystem.h>
#include <Quark/Core/Input.h>
#include <Quark/Core/Logger.h>
#include <Quark/Events/KeyEvent.h>
#include <Quark/Events/EventManager.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Scene/SceneSerializer.h>
#include <Quark/Asset/TextureImporter.h>
#include <Quark/Asset/AssetManager.h>
#include <Quark/Asset/MaterialSerializer.h>
#include <Quark/Renderer/Renderer.h>
#include <Quark/UI/UI.h>

namespace quark {

Application* CreateApplication()
{    
    ApplicationSpecification specs;
    specs.uiSpecs.flags = UI_INIT_FLAG_DOCKING | UI_INIT_FLAG_VIEWPORTS;
    specs.title = "Quark Editor";
    specs.width = 1600;
    specs.height = 1000;
    specs.isFullScreen = false;

    return new EditorApp(specs);
}

EditorApp::EditorApp(const ApplicationSpecification& specs)
    : Application(specs), m_ViewportFocused(false), m_ViewportHovered(false), m_EditorCamera(60, 1280, 720, 0.1f, 256), m_ViewportSize(1000, 800) // dont'care here, will be overwrited
{
    // Create Render structures
    m_ForwardPassInfo = Renderer::Get().renderPassInfo2_simpleColorDepthPass;   // use defalut render pass
    m_UiPassInfo = Renderer::Get().renderPassInfo2_uiPass;
    CreateColorDepthAttachments();

    // Load cube map
    TextureImporter textureLoader;
    m_CubeMapTexture = textureLoader.ImportKtx2("BuiltInResources/Textures/Cubemaps/etc1s_cubemap_learnopengl.ktx2", true);

    // Load scene
    m_Scene = CreateScope<Scene>("");

    // Init UI Panels
    m_HeirarchyPanel.SetScene(m_Scene);
    m_InspectorPanel.SetScene(m_Scene);

    // SetUp Renderer
    m_SceneRenderer = CreateScope<SceneRenderer>(m_GraphicDevice.get());
    m_SceneRenderer->SetScene(m_Scene);
    m_SceneRenderer->SetCubeMap(m_CubeMapTexture);

    // Adjust editor camera's aspect ratio
    m_EditorCamera.viewportWidth = (float)Application::Get().GetWindow()->GetWidth();
    m_EditorCamera.viewportHeight = (float)Application::Get().GetWindow()->GetHeight();
    m_EditorCamera.SetPosition(glm::vec3(0, 10, 10));

    EventManager::Get().Subscribe<KeyPressedEvent>([&](const KeyPressedEvent& e) { OnKeyPressed(e); });
}

EditorApp::~EditorApp()
{   
    // Save asset registry
    AssetManager::Get().SaveAssetRegistry();

}

void EditorApp::OnUpdate(TimeStep ts)
{   
    // Update Editor camera's aspect ratio
    m_EditorCamera.viewportWidth = m_ViewportSize.x;
    m_EditorCamera.viewportHeight = m_ViewportSize.y;

    // Update Editor camera's movement
    if (m_ViewportHovered && Input::Get()->IsKeyPressed(Key::LeftAlt, true))
        m_EditorCamera.OnUpdate(ts);

    // TODO: Update physics

    // Update scene
    m_Scene->OnUpdate();

}   

void EditorApp::OnImGuiUpdate()
{
    UI::Get()->BeginFrame();

    // Update Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                NewScene();

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                OpenScene();

            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                SaveSceneAs();

            //if (ImGui::MenuItem("Exit"))

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Debug Ui
    if (ImGui::Begin("Debug")) 
    {
        ImGui::Text("FPS: %f", m_Status.fps);
        ImGui::Text("Frame Time: %f ms", m_Status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", m_CmdListRecordTime);
    }
    ImGui::End();

    // Scene Heirarchy
    m_HeirarchyPanel.OnImGuiUpdate();
    
    // Inspector
    m_InspectorPanel.SetSelectedEntity(m_HeirarchyPanel.GetSelectedEntity());
    m_InspectorPanel.OnImGuiUpdate();

    // Content Browser
    m_ContentBrowserPanel.OnImGuiUpdate();

    // Scene view port
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport");

    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

    ImTextureID colorAttachmentId = UI::Get()->GetOrCreateTextureId(m_color_attachment, Renderer::Get().sampler_linear);
    ImGui::Image(colorAttachmentId, ImVec2{ m_ViewportSize.x, m_ViewportSize.y });

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* path = (const char*)payload->Data;
            OpenScene(path);
        }

        ImGui::EndDragDropTarget();
    }

    // Gizmos
    Entity* selectedEntity = m_HeirarchyPanel.GetSelectedEntity();
    if (selectedEntity && m_GizmoType != -1)
    {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        glm::mat4 cameraProjection = m_EditorCamera.GetProjectionMatrix();
        glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

        auto* tc = selectedEntity->GetComponent<TransformCmpt>();
        glm::mat4 transform = tc->GetLocalMatrix();

        bool snap = Input::Get()->IsKeyPressed(Key::LeftControl, true);
        float snapValue = 0.5f; // Snap to 0.5m for translation/scale
        // Snap to 45 degrees for rotation
        if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
            snapValue = 45.0f;

        float snapValues[3] = { snapValue, snapValue, snapValue };

        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
            (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
            nullptr, snap ? snapValues : nullptr);

        if (ImGuizmo::IsUsing())
        {
            glm::vec3 translation, rotation, scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));

            tc->SetLocalPosition(translation);
            tc->SetLocalRotate(glm::radians(rotation));
            tc->SetLocalScale(scale);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    UI::Get()->EndFrame();
}

void EditorApp::NewScene()
{
    m_Scene = CreateRef<Scene>("New Scene");

    m_SceneRenderer->SetScene(m_Scene);
    m_HeirarchyPanel.SetScene(m_Scene);
    m_InspectorPanel.SetScene(m_Scene);
}

void EditorApp::OpenScene()
{
    std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "Quark Scene", "qkscene" } });
    if (!filepath.empty())
        OpenScene(filepath);
}

void EditorApp::OpenScene(const std::filesystem::path& path)
{
    m_Scene = CreateRef<Scene>("");
    SceneSerializer serializer(m_Scene);
    serializer.Deserialize(path.string());

    m_SceneRenderer->SetScene(m_Scene);
    m_HeirarchyPanel.SetScene(m_Scene);
    m_InspectorPanel.SetScene(m_Scene);
}

void EditorApp::SaveSceneAs()
{
    std::filesystem::path filepath = FileSystem::SaveFileDialog({ { "Quark Scene", "qkscene" } });
    if (!filepath.empty())
    {
        SceneSerializer serializer(m_Scene);
        serializer.Serialize(filepath.string());
    }
}

void EditorApp::OnRender(TimeStep ts)
{
    // Sync the rendering data with game scene
    CameraUniformBufferBlock cameraData;
    cameraData.proj = m_EditorCamera.GetProjectionMatrix();
    cameraData.proj[1][1] *= -1;
    cameraData.view = m_EditorCamera.GetViewMatrix();
    cameraData.viewproj = cameraData.proj * cameraData.view;

    m_SceneRenderer->UpdateDrawContext(cameraData);

    // Rendering commands
    auto graphic_device = Application::Get().GetGraphicDevice();

    if (graphic_device->BeiginFrame(ts)) {
        auto* cmd = graphic_device->BeginCommandList();
        auto* swap_chain_image = graphic_device->GetPresentImage();

        // Geometry pass
        {
            graphic::PipelineImageBarrier image_barrier;
            image_barrier.image = m_color_attachment.get();
            image_barrier.srcStageBits = graphic::PIPELINE_STAGE_ALL_GRAPHICS_BIT;
            image_barrier.srcMemoryAccessBits = 0;
            image_barrier.dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barrier.layoutBefore = graphic::ImageLayout::UNDEFINED;
            image_barrier.layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &image_barrier, 1, nullptr, 0);

            // Viewport and scissor
            graphic::Viewport viewport;
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = (float)m_color_attachment->GetDesc().width;
            viewport.height = (float)m_color_attachment->GetDesc().height;
            viewport.minDepth = 0;
            viewport.maxDepth = 1;

            graphic::Scissor scissor;
            scissor.extent.width = (float)viewport.width;
            scissor.extent.height = (float)viewport.height;
            scissor.offset.x = 0;
            scissor.offset.y = 0;

            // Begin pass
            graphic::FrameBufferInfo fb_info;
            fb_info.colorAttatchemtsLoadOp[0] = graphic::FrameBufferInfo::AttachmentLoadOp::CLEAR;
            fb_info.colorAttatchemtsStoreOp[0] = graphic::FrameBufferInfo::AttachmentStoreOp::STORE;
            fb_info.colorAttachments[0] = { m_color_attachment.get() };
            fb_info.clearColors[0] = { 0.22f, 0.22f, 0.22f, 1.f };
            fb_info.depthAttachment = m_depth_attachment.get();
            fb_info.depthAttachmentLoadOp = graphic::FrameBufferInfo::AttachmentLoadOp::CLEAR;
            fb_info.depthAttachmentStoreOp = graphic::FrameBufferInfo::AttachmentStoreOp::STORE;
            fb_info.clearDepthStencil.depth_stencil = { 1.f, 0 };

            cmd->BeginRenderPass(m_ForwardPassInfo, fb_info);
            cmd->SetViewPort(viewport);
            cmd->SetScissor(scissor);

            // Draw skybox
            m_SceneRenderer->RenderSkybox(cmd);

            // Draw scene
            auto geometry_start = m_Timer.ElapsedMillis();
            m_SceneRenderer->RenderScene(cmd);
            m_CmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

            cmd->EndRenderPass();
        }

        // UI pass
        {
            graphic::PipelineImageBarrier swapchain_image_barrier;
            swapchain_image_barrier.image = swap_chain_image;
            swapchain_image_barrier.srcStageBits = graphic::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            swapchain_image_barrier.srcMemoryAccessBits = 0;
            swapchain_image_barrier.dstStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            swapchain_image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            swapchain_image_barrier.layoutBefore = graphic::ImageLayout::UNDEFINED;
            swapchain_image_barrier.layoutAfter = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &swapchain_image_barrier, 1, nullptr, 0);

            graphic::PipelineImageBarrier color_attachment_barrier;
            color_attachment_barrier.image = m_color_attachment.get();
            color_attachment_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            color_attachment_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            color_attachment_barrier.dstStageBits = graphic::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            color_attachment_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_SHADER_READ_BIT;
            color_attachment_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            color_attachment_barrier.layoutAfter = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &color_attachment_barrier, 1, nullptr, 0);

            graphic::FrameBufferInfo fb_info;
            fb_info.colorAttatchemtsLoadOp[0] = graphic::FrameBufferInfo::AttachmentLoadOp::CLEAR;
            fb_info.colorAttatchemtsStoreOp[0] = graphic::FrameBufferInfo::AttachmentStoreOp::STORE;
            fb_info.colorAttachments[0] = swap_chain_image;

            cmd->BeginRenderPass(m_UiPassInfo, fb_info);
            UI::Get()->OnRender(cmd);
            cmd->EndRenderPass();
        }


        // Transit swapchain image to present layout for presenting
        {
            graphic::PipelineImageBarrier present_barrier;
            present_barrier.image = swap_chain_image;
            present_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            present_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            present_barrier.dstStageBits = graphic::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            present_barrier.dstMemoryAccessBits = 0;
            present_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            present_barrier.layoutAfter = graphic::ImageLayout::PRESENT;
            cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);
        }

        // Submit command list
        graphic_device->SubmitCommandList(cmd);
        graphic_device->EndFrame(ts);
    }
}


void EditorApp::OnKeyPressed(const KeyPressedEvent& e)
{
    if (e.repeatCount > 0)
        return;

    bool control = Input::Get()->IsKeyPressed(Key::LeftControl,true) || Input::Get()->IsKeyPressed(Key::RightControl, true);
    bool shift = Input::Get()->IsKeyPressed(Key::LeftShift, true) || Input::Get()->IsKeyPressed(Key::RightShift, true);

    switch (e.key)
    {
    case Key::N:
		if (control)
			NewScene();
		break;
    case Key::O:
        if (control)
            OpenScene();
        break;
    case Key::S:
		if (control && shift)
			SaveSceneAs();
		break;
    // Gizmos
    case Key::Q:
        if (!ImGuizmo::IsUsing())
            m_GizmoType = -1;
        break;
    case Key::W:
        if (!ImGuizmo::IsUsing())
			m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		break;
    case Key::E:
		if (!ImGuizmo::IsUsing())
            m_GizmoType = ImGuizmo::OPERATION::ROTATE;
        break;
    case Key::R:
        if (!ImGuizmo::IsUsing())
		m_GizmoType = ImGuizmo::OPERATION::SCALE;
        break;
    default:
        break;
    }
}

void EditorApp::CreateColorDepthAttachments()
{
        using namespace quark::graphic;

        // Create depth image
        ImageDesc image_desc;
        image_desc.type = ImageType::TYPE_2D;
        image_desc.width = uint32_t(Application::Get().GetWindow()->GetMonitorWidth() * Application::Get().GetWindow()->GetRatio());
        image_desc.height = uint32_t(Application::Get().GetWindow()->GetMonitorHeight() * Application::Get().GetWindow()->GetRatio());
        image_desc.depth = 1;
        image_desc.format = Renderer::Get().format_depthAttachment_main;
        image_desc.arraySize = 1;
        image_desc.mipLevels = 1;
        image_desc.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        image_desc.usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        m_depth_attachment = m_GraphicDevice->CreateImage(image_desc);

        // Create color image
        image_desc.format = Renderer::Get().format_colorAttachment_main;
        image_desc.initialLayout = ImageLayout::UNDEFINED;
        image_desc.usageBits = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | graphic::IMAGE_USAGE_SAMPLING_BIT;
        m_color_attachment = m_GraphicDevice->CreateImage(image_desc);
}

}