#include "Editor/EditorApp.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

#include <Quark/Core/Window.h>
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
#include <Quark/Renderer/GpuResourceManager.h>
#include <Quark/Renderer/GLSLCompiler.h>
#include <Quark/UI/UI.h>

namespace quark {
Application* CreateApplication()
{    
    AppInitSpecs specs;
    specs.uiSpecs.flags = UI_INIT_FLAG_DOCKING | UI_INIT_FLAG_VIEWPORTS;
    specs.title = "Quark Editor";
    specs.width = 1600;
    specs.height = 1000;
    specs.isFullScreen = false;

    return new EditorApp(specs);
}

EditorApp::EditorApp(const AppInitSpecs& specs)
    : Application(specs), m_ViewportFocused(false), m_ViewportHovered(false), m_EditorCamera(60, 1280, 720, 0.1, 256), m_ViewportSize(1000, 800) // dont'care here, will be overwrited
{
    color_format = m_GraphicDevice->GetSwapChainImageFormat();

    // Create Render structures
    SetUpRenderPass();
    CreatePipeline();
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
    m_EditorCamera.viewportWidth = Window::Instance()->GetWidth();
    m_EditorCamera.viewportHeight = Window::Instance()->GetHeight();
    m_EditorCamera.SetPosition(glm::vec3(0, 10, 10));


    EventManager::Instance().Subscribe<KeyPressedEvent>([&](const KeyPressedEvent& e) {
        OnKeyPressed(e);
    });
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
        ImGui::Text("CmdList Record Time: %f ms", cmdListRecordTime);
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
    ImGui::Image(m_ColorAttachmentId, ImVec2{ m_ViewportSize.x, m_ViewportSize.y });

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
        glm::mat4 transform = tc->GetWorldMatrix();

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

            tc->SetPosition(translation);
            tc->SetEuler(glm::radians(rotation));
            tc->SetScale(scale);
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
    {
        OpenScene(filepath);
    }

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
        auto cmd = graphic_device->BeginCommandList();
        auto* swap_chain_image = graphic_device->GetPresentImage();

        // Geometry pass
        {
            graphic::PipelineImageBarrier image_barrier;
            image_barrier.image = color_image.get();
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
            viewport.width = (float)color_image->GetDesc().width;
            viewport.height = (float)color_image->GetDesc().height;
            viewport.minDepth = 0;
            viewport.maxDepth = 1;

            graphic::Scissor scissor;
            scissor.extent.width = viewport.width;
            scissor.extent.height = viewport.height;
            scissor.offset.x = 0;
            scissor.offset.y = 0;

            // Begin pass
            forward_pass_info.colorAttachments[0] = color_image.get();
            forward_pass_info.clearColors[0] = {0.22f, 0.22f, 0.22f, 1.f};
            forward_pass_info.depthAttachment = depth_image.get();
            cmd->BeginRenderPass(forward_pass_info);

            // Draw skybox
            //cmd->BindPipeLine(*skybox_pipeline);
            //cmd->SetViewPort(viewport);
            //cmd->SetScissor(scissor);
            //m_SceneRenderer->RenderSkybox(cmd);

            // Draw scene
            cmd->BindPipeLine(*graphic_pipeline);
            cmd->SetViewPort(viewport);
            cmd->SetScissor(scissor);
            auto geometry_start = m_Timer.ElapsedMillis();
            m_SceneRenderer->RenderScene(cmd);
            cmdListRecordTime = m_Timer.ElapsedMillis() - geometry_start;

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

            graphic::PipelineImageBarrier color_image_barrier;
            color_image_barrier.image = color_image.get();
            color_image_barrier.srcStageBits = graphic::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            color_image_barrier.srcMemoryAccessBits = graphic::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            color_image_barrier.dstStageBits = graphic::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            color_image_barrier.dstMemoryAccessBits = graphic::BARRIER_ACCESS_SHADER_READ_BIT;
            color_image_barrier.layoutBefore = graphic::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            color_image_barrier.layoutAfter = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            cmd->PipeLineBarriers(nullptr, 0, &color_image_barrier, 1, nullptr, 0);

            ui_pass_info.colorAttachments[0] = swap_chain_image;
            cmd->BeginRenderPass(ui_pass_info);
            UI::Get()->Render(cmd);
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

void EditorApp::CreatePipeline()
{
    using namespace quark::graphic;

    // Sky box shaders
    skybox_vert_shader = m_GraphicDevice->CreateShaderFromSpvFile(graphic::ShaderStage::STAGE_VERTEX, "BuiltInResources/Shaders/Spirv/skybox.vert.spv");
    skybox_frag_shader = m_GraphicDevice->CreateShaderFromSpvFile(graphic::ShaderStage::STAGE_FRAGEMNT, "BuiltInResources/Shaders/Spirv/skybox.frag.spv");

    // Scene shaders
    GLSLCompiler compiler;
    compiler.SetTarget(GLSLCompiler::Target::VULKAN_VERSION_1_3);
    compiler.SetSourceFromFile("BuiltInResources/Shaders/mesh.vert", ShaderStage::STAGE_VERTEX);
    std::vector<uint32_t> spvCode;
    std::string messages;
    if (!compiler.Compile(messages, spvCode, {}))
        LOGE(messages);

    vert_shader = m_GraphicDevice->CreateShaderFromBytes(ShaderStage::STAGE_VERTEX, spvCode.data(), spvCode.size() * sizeof(uint32_t));

    spvCode.clear();
    messages.clear();
    compiler.SetSourceFromFile("BuiltInResources/Shaders/mesh.frag", ShaderStage::STAGE_FRAGEMNT);
    if (!compiler.Compile(messages, spvCode, {}))
        LOGE(messages);
    frag_shader = m_GraphicDevice->CreateShaderFromBytes(ShaderStage::STAGE_FRAGEMNT, spvCode.data(), spvCode.size() * sizeof(uint32_t));
    
    // Scene pipeline
    GraphicPipeLineDesc pipe_desc;
    pipe_desc.vertShader = vert_shader;
    pipe_desc.fragShader = frag_shader;
    pipe_desc.blendState = PipelineColorBlendState::create_disabled(1);
    pipe_desc.topologyType = TopologyType::TRANGLE_LIST;
    pipe_desc.renderPassInfo = forward_pass_info;
    pipe_desc.depthStencilState.enableDepthTest = true;
    pipe_desc.depthStencilState.enableDepthWrite = true;
    pipe_desc.depthStencilState.depthCompareOp = CompareOperation::LESS_OR_EQUAL;
    pipe_desc.rasterState.cullMode = CullMode::NONE;
    pipe_desc.rasterState.polygonMode = PolygonMode::Fill;
    pipe_desc.rasterState.frontFaceType = FrontFaceType::COUNTER_CLOCKWISE;
    graphic_pipeline = m_GraphicDevice->CreateGraphicPipeLine(pipe_desc);

    // Sky box pipeline
    pipe_desc.vertShader = skybox_vert_shader;
    pipe_desc.fragShader = skybox_frag_shader;
    pipe_desc.depthStencilState.enableDepthTest = false;
    pipe_desc.depthStencilState.enableDepthWrite = false;

    VertexBindInfo vert_bind_info;
    vert_bind_info.binding = 0;
    vert_bind_info.stride = sizeof(Vertex);
    vert_bind_info.inputRate = VertexBindInfo::INPUT_RATE_VERTEX;
    pipe_desc.vertexBindInfos.push_back(vert_bind_info);

    VertexAttribInfo pos_attrib;
    pos_attrib.binding = 0;
    pos_attrib.format = VertexAttribInfo::ATTRIB_FORMAT_VEC3;
    pos_attrib.location = 0;
    pos_attrib.offset = offsetof(Vertex, position);
    pipe_desc.vertexAttribInfos.push_back(pos_attrib);

    skybox_pipeline = m_GraphicDevice->CreateGraphicPipeLine(pipe_desc);
}   

void EditorApp::CreateColorDepthAttachments()
{
        using namespace quark::graphic;

        // Create depth image
        ImageDesc image_desc;
        image_desc.type = ImageType::TYPE_2D;
        image_desc.width = uint32_t(Window::Instance()->GetMonitorWidth() * Window::Instance()->GetRatio());
        image_desc.height = uint32_t(Window::Instance()->GetMonitorHeight() * Window::Instance()->GetRatio());
        image_desc.depth = 1;
        image_desc.format = depth_format;
        image_desc.arraySize = 1;
        image_desc.mipLevels = 1;
        image_desc.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        image_desc.usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depth_image = m_GraphicDevice->CreateImage(image_desc);

        // Create color image
        image_desc.format = color_format;
        image_desc.initialLayout = ImageLayout::UNDEFINED;
        image_desc.usageBits = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | graphic::IMAGE_USAGE_SAMPLING_BIT;
        color_image = m_GraphicDevice->CreateImage(image_desc);

        // Create Imgui texture id
        m_ColorAttachmentId = UI::Get()->CreateTextureId(*color_image, *GpuResourceManager::Get().linearSampler);
}

void EditorApp::SetUpRenderPass()
{   
    // First pass : geometry pass
    forward_pass_info = {};
    forward_pass_info.numColorAttachments = 1;
    forward_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    forward_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    forward_pass_info.colorAttachmentFormats[0] = color_format;
    forward_pass_info.depthAttachment = depth_image.get();
    forward_pass_info.depthAttachmentLoadOp = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    forward_pass_info.depthAttachmentStoreOp = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    forward_pass_info.ClearDepthStencil.depth_stencil = {1.f, 0};
    forward_pass_info.depthAttachmentFormat = depth_format;

    // Second pass : UI pass
    ui_pass_info = {};
    ui_pass_info.numColorAttachments = 1;
    ui_pass_info.colorAttatchemtsLoadOp[0] = graphic::RenderPassInfo::AttachmentLoadOp::CLEAR;
    ui_pass_info.colorAttatchemtsStoreOp[0] = graphic::RenderPassInfo::AttachmentStoreOp::STORE;
    ui_pass_info.colorAttachmentFormats[0] = m_GraphicDevice->GetSwapChainImageFormat();

}

}