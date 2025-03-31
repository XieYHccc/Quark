#include "Editor/EditorApp.h"

#include <Quark/Quark.h>
#include <Quark/EntryPoint.h>
#include <Quark/Asset/ImageImporter.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace quark {

Application* CreateApplication(int argc, char** argv)
{   
    ApplicationSpecification specs;
    specs.uiSpecs.flags = UI_INIT_FLAG_DOCKING | UI_INIT_FLAG_VIEWPORTS;
    specs.title = "Quark Editor";
    specs.width = 2500;
    specs.height = 1600;
    specs.isFullScreen = false;
    specs.workingDirectory = "D:/Dev/Quark/bin";

    return new EditorApp(specs);
}

EditorApp::EditorApp(const ApplicationSpecification& specs)
    : Application(specs), m_viewportFocused(false), m_viewportHovered(false), 
    m_editorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f), m_viewportSize(1000, 800), // dont'care here, will be overwrited
    m_hoverdEntity(nullptr)
{
    CreateGraphicResources();

    GLTFImporter importer(RenderSystem::Get().GetDevice());
    importer.Import("BuiltInResources/Gltf/CesiumMan/glTF/CesiumMan.gltf", GLTFImporter::ImportAll);
    Ref<SkeletonAsset> skeleton = importer.GetSkeletons()[0];
    Ref<AnimationAsset> animation = importer.GetAnimations()[0];
    AssetManager::Get().AddMemoryOnlyAsset(skeleton);
    AssetManager::Get().AddMemoryOnlyAsset(animation);

    // load cube map
    ImageImporter imageLoader;
    Ref<ImageAsset> cubeMap = imageLoader.ImportKtx2("BuiltInResources/Textures/Cubemaps/etc1s_cubemap_learnopengl.ktx2", true);
    m_cubeMapId = cubeMap->GetAssetID();
    AssetManager::Get().AddMemoryOnlyAsset(cubeMap);

    // Ref<Project> project = CreateRef<Project>();
    // ProjectSerializer serializer(project);
    // serializer.Serialize("SandboxProject/Sandbox.qkproject");

    OpenProject("SandboxProject/Sandbox.qkproject");

    Entity* cesium_entity = m_scene->CreateEntity("Cesium");
    m_scene->AddArmatureComponent(cesium_entity, skeleton);
    auto* anim_cmpt = cesium_entity->AddComponent<AnimationCmpt>();
    anim_cmpt->animation_asset_id = animation->GetAssetID();

    m_gltfImporter.Import("BuiltInResources/Gltf/CesiumMan/glTF/CesiumMan.gltf", GLTFImporter::ImportAll);
    m_gltfImporter.AddAllAssetsAsMemoryOnly();

    m_scene = m_gltfImporter.GetScene();
    m_heirarchyPanel.SetScene(m_scene);
    m_inspectorPanel.SetScene(m_scene);
    m_hoverdEntity = nullptr;

    // register event callbacks
    EventManager::Get().Subscribe<KeyPressedEvent>([&](const KeyPressedEvent& e) { OnKeyPressed(e); });
    EventManager::Get().Subscribe<MouseButtonPressedEvent>([&](const MouseButtonPressedEvent& e) { OnMouseButtonPressed(e); });
    
}

EditorApp::~EditorApp()
{   
    // save asset registry
    // AssetManager::Get().SaveAssetRegistry();
}

void EditorApp::OnUpdate(TimeStep ts)
{   
    // update Editor camera's aspect ratio and movement
    m_editorCamera.viewportWidth = m_viewportSize.x;
    m_editorCamera.viewportHeight = m_viewportSize.y;
    if (m_viewportHovered && Input::Get()->IsKeyPressed(Key::LeftAlt, true))
        m_editorCamera.OnUpdate(ts);

    // update scene
    m_scene->OnUpdate(ts);

    // TODO: Update physics

    // update entity picking
    if (m_viewportHovered)
    {
        uint32_t* pixel = (uint32_t*)m_stage_buffer->GetMappedDataPtr();
        uint32_t low = pixel[0];
        uint32_t high = pixel[1];

        if (low != 0 && high != 0)
        {
            uint64_t entityID = ((uint64_t)high << 32) | (uint64_t)low;
            m_hoverdEntity = m_scene->GetEntityWithID(entityID);
        }
        else
        {
            m_hoverdEntity = nullptr;
        }
    }

    // Sync the rendering data with game scene
    UniformBufferObject_Camera cameraData;
    cameraData.proj = m_editorCamera.GetProjectionMatrix();
    cameraData.proj[1][1] *= -1;
    cameraData.view = m_editorCamera.GetViewMatrix();
    cameraData.viewproj = cameraData.proj * cameraData.view;

    auto& swap_data = RenderSystem::Get().GetSwapContext().GetLogicSwapData();
    m_scene->FillMeshSwapData();
    CameraSwapData cam_swap_data;
    cam_swap_data.proj = cameraData.proj;
    cam_swap_data.view = cameraData.view;
    swap_data.camera_swap_data = cam_swap_data;

    // swap render and logic data
    // RenderSystem::Get().UpdateDrawContext(m_scene, m_drawContext);
    // RenderSystem::Get().UpdateVisibility(m_drawContext, m_visibility, cameraData);

    RenderSystem::Get().GetSwapContext().SwapLogicRenderData();
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

            if (ImGui::MenuItem("Open Project..."))
				OpenProject();

            //if (ImGui::MenuItem("Exit"))

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Debug Ui
    if (ImGui::Begin("Debug")) 
    {
        ImGui::Text("FPS: %f", m_status.fps);
        ImGui::Text("Frame Time: %f ms", m_status.lastFrameDuration);
        ImGui::Text("CmdList Record Time: %f ms", m_cmdListRecordTime);

        std::string entityName = "None";
        if (m_hoverdEntity)
            entityName = m_hoverdEntity->GetComponent<NameCmpt>()->name;

		ImGui::Text("Hovered Entity: %s", entityName.c_str());
    }
    ImGui::End();

    // Scene Heirarchy
    m_heirarchyPanel.OnImGuiUpdate();
    
    // Inspector
    m_inspectorPanel.SetSelectedEntity(m_heirarchyPanel.GetSelectedEntity());
    m_inspectorPanel.OnImGuiUpdate();

    // Content Browser
    m_contentBrowserPanel.OnImGuiUpdate();

    // Scene view port
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport");

    auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
    auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
    auto viewportOffset = ImGui::GetWindowPos();
    m_viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    m_viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

    ImTextureID colorAttachmentId = UI::Get()->GetOrCreateTextureId(m_color_attachment, RenderSystem::Get().GetRenderResourceManager().sampler_linear);
    ImGui::Image(colorAttachmentId, ImVec2{ m_viewportSize.x, m_viewportSize.y });


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
    Entity* selectedEntity = m_heirarchyPanel.GetSelectedEntity();
    if (selectedEntity && m_gizmoType != -1)
    {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        glm::mat4 cameraProjection = m_editorCamera.GetProjectionMatrix();
        glm::mat4 cameraView = m_editorCamera.GetViewMatrix();

        auto* tc = selectedEntity->GetComponent<TransformCmpt>();
        glm::mat4 transform = tc->GetLocalMatrix();

        bool snap = Input::Get()->IsKeyPressed(Key::LeftControl, true);
        float snapValue = 0.5f; // Snap to 0.5m for translation/scale
        // Snap to 45 degrees for rotation
        if (m_gizmoType == ImGuizmo::OPERATION::ROTATE)
            snapValue = 45.0f;

        float snapValues[3] = { snapValue, snapValue, snapValue };

        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
            (ImGuizmo::OPERATION)m_gizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
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
    m_scene = CreateRef<Scene>("New Scene");

    m_heirarchyPanel.SetScene(m_scene);
    m_inspectorPanel.SetScene(m_scene);
}

void EditorApp::OpenScene()
{
    std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "Quark Scene", "qkscene" } });
    if (!filepath.empty())
        OpenScene(filepath);
}

void EditorApp::OpenScene(const std::filesystem::path& path)
{
    m_scene = CreateRef<Scene>("");
    SceneSerializer serializer(m_scene);
    serializer.Deserialize(path.string());

    m_heirarchyPanel.SetScene(m_scene);
    m_inspectorPanel.SetScene(m_scene);

    m_hoverdEntity = nullptr;
}

void EditorApp::SaveSceneAs()
{
    std::filesystem::path filepath = FileSystem::SaveFileDialog({ { "Quark Scene", "qkscene" } });
    if (!filepath.empty())
    {
        SceneSerializer serializer(m_scene);
        serializer.Serialize(filepath.string());
    }
}

bool EditorApp::OpenProject(const std::filesystem::path& filepath)
{
    if (!FileSystem::Exists(filepath))
    {
        QK_CORE_LOGE_TAG("Editor", "Tried to open a project that doesn't exist. Project path: {0}", filepath.string());
        return false;
    }

    if (auto active_proj = Project::GetActive())
        active_proj.reset();

    Ref<Project> newProject = CreateRef<Project>();
    ProjectSerializer serializer(newProject);
    serializer.Deserialize(filepath);
    Project::SetActive(newProject);

    m_contentBrowserPanel.Init(newProject->GetAssetDirectory());

    auto path = newProject->GetStartScenePath();
    bool hasStartScene = FileSystem::Exists(newProject->GetStartScenePath());

    if (hasStartScene)
		OpenScene(newProject->GetStartScenePath());

    m_editorCamera = EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f);
    return true;
}

bool EditorApp::OpenProject()
{
    std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "Quark Project", "qkproject" } });
    if (filepath.empty())
        return false;

    OpenProject(filepath);
    return true;
}

void EditorApp::OnRender(TimeStep ts)
{
    auto& render_system = RenderSystem::Get();
    Ref<rhi::Device> rhi_device = render_system.GetDevice();
    auto render_scene = render_system.GetRenderScene(); //TODO: remove this
    render_system.ProcessSwapData();
    
    // Rendering commands
    if (rhi_device->BeiginFrame(ts)) 
    {
        // render_system.UpdateGpuResources(m_drawContext, m_visibility);

        auto* graphic_cmd = rhi_device->BeginCommandList();
        auto* swap_chain_image = rhi_device->GetPresentImage();

        // Viewport and scissor
        rhi::Viewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)m_color_attachment->GetDesc().width;
        viewport.height = (float)m_color_attachment->GetDesc().height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;

        rhi::Scissor scissor;
        scissor.extent.width = (int)viewport.width;
        scissor.extent.height = (int)viewport.height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;

        // Main pass
        {
            rhi::PipelineImageBarrier image_barrier;
            image_barrier.image = m_color_attachment.get();
            image_barrier.srcStageBits = rhi::PIPELINE_STAGE_ALL_GRAPHICS_BIT;
            image_barrier.srcMemoryAccessBits = 0;
            image_barrier.dstStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barrier.dstMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT | rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barrier.layoutBefore = rhi::ImageLayout::UNDEFINED;
            image_barrier.layoutAfter = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            graphic_cmd->PipeLineBarriers(nullptr, 0, &image_barrier, 1, nullptr, 0);

            // Begin pass
            rhi::FrameBufferInfo fb_info;
            fb_info.colorAttatchemtsLoadOp[0] = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
            fb_info.colorAttatchemtsStoreOp[0] = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
            fb_info.colorAttachments[0] = m_color_attachment.get();
            fb_info.clearColors[0] = { 0.2f, 0.2f, 0.2f, 0.f };
            fb_info.depthAttachment = m_depth_attachment.get();
            fb_info.depthAttachmentLoadOp = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
            fb_info.depthAttachmentStoreOp = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
            fb_info.clearDepthStencil.depth_stencil = { 1.f, 0 };

            graphic_cmd->BeginRenderPass(render_system.GetRenderResourceManager().renderPassInfo_simpleMainPass, fb_info);
            graphic_cmd->SetViewPort(viewport);
            graphic_cmd->SetScissor(scissor);

            // draw skybox
            // render_system.DrawSkybox(m_drawContext, m_cubeMapTexture, graphic_cmd);
            render_system.DrawSkybox(m_cubeMapId, graphic_cmd);
            
            // draw infinite grid
            // render_system.DrawGrid(graphic_cmd);

            // draw scene
            auto geometry_start = m_timer.ElapsedMillis();
            render_system.DrawScene(*render_scene, render_scene->main_camera_visibility, graphic_cmd);
            m_cmdListRecordTime = m_timer.ElapsedMillis() - geometry_start;

            graphic_cmd->EndRenderPass();
        }

        // ui pass
        {
            rhi::PipelineImageBarrier image_barriers[2];
            image_barriers[0].image = swap_chain_image;
            image_barriers[0].srcStageBits = rhi::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            image_barriers[0].srcMemoryAccessBits = 0;
            image_barriers[0].dstStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barriers[0].dstMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barriers[0].layoutBefore = rhi::ImageLayout::UNDEFINED;
            image_barriers[0].layoutAfter = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            image_barriers[1].image = m_color_attachment.get();
            image_barriers[1].srcStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barriers[1].srcMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barriers[1].dstStageBits = rhi::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            image_barriers[1].dstMemoryAccessBits = rhi::BARRIER_ACCESS_SHADER_READ_BIT;
            image_barriers[1].layoutBefore = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            image_barriers[1].layoutAfter = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            graphic_cmd->PipeLineBarriers(nullptr, 0, image_barriers, 2, nullptr, 0);

            rhi::FrameBufferInfo fb_info;
            fb_info.colorAttatchemtsLoadOp[0] = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
            fb_info.colorAttatchemtsStoreOp[0] = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
            fb_info.colorAttachments[0] = swap_chain_image;

            graphic_cmd->BeginRenderPass(render_system.GetRenderResourceManager().renderPassInfo_swapchainPass, fb_info);
            UI::Get()->OnRender(graphic_cmd);
            graphic_cmd->EndRenderPass();
        }

        // transit swapchain image to present layout for presenting
        {
            rhi::PipelineImageBarrier present_barrier;
            present_barrier.image = swap_chain_image;
            present_barrier.srcStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            present_barrier.srcMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            present_barrier.dstStageBits = rhi::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            present_barrier.dstMemoryAccessBits = 0;
            present_barrier.layoutBefore = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            present_barrier.layoutAfter = rhi::ImageLayout::PRESENT;
            graphic_cmd->PipeLineBarriers(nullptr, 0, &present_barrier, 1, nullptr, 0);

            // Submit graphic command list
            rhi_device->SubmitCommandList(graphic_cmd);
        }

         // color picking
         if (m_viewportHovered)
         {
             // color ID pass
             rhi::CommandList* cmd = rhi_device->BeginCommandList();

             rhi::PipelineImageBarrier image_barrier;
             image_barrier.image = m_entityID_color_attachment.get();
             image_barrier.srcStageBits = rhi::PIPELINE_STAGE_TRANSFER_BIT;
             image_barrier.srcMemoryAccessBits = rhi::BARRIER_ACCESS_TRANSFER_READ_BIT;
             image_barrier.dstStageBits = rhi::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
             image_barrier.dstMemoryAccessBits = rhi::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
             image_barrier.layoutBefore = rhi::ImageLayout::UNDEFINED;
             image_barrier.layoutAfter = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
             cmd->PipeLineBarriers(nullptr, 0, &image_barrier, 1, nullptr, 0);

             rhi::FrameBufferInfo fb_info;
             fb_info.colorAttatchemtsLoadOp[0] = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
             fb_info.colorAttatchemtsStoreOp[0] = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
             fb_info.colorAttachments[0] = m_entityID_color_attachment.get();
             fb_info.clearColors[0].color.uint32[0] = 0;
             fb_info.clearColors[0].color.uint32[1] = 10;
             fb_info.depthAttachment = m_entityID_depth_attachment.get();
             fb_info.depthAttachmentLoadOp = rhi::FrameBufferInfo::AttachmentLoadOp::CLEAR;
             fb_info.depthAttachmentStoreOp = rhi::FrameBufferInfo::AttachmentStoreOp::STORE;
             fb_info.clearDepthStencil.depth_stencil = { 1.f, 0 };

             cmd->BeginRenderPass(render_system.GetRenderResourceManager().renderPassInfo_entityIdPass, fb_info);
             cmd->SetViewPort(viewport);
             cmd->SetScissor(scissor);
             render_system.DrawEntityID(*render_scene, render_scene->main_camera_visibility, cmd);
             cmd->EndRenderPass();

             // transfer data back to cpu buffer
             rhi::PipelineImageBarrier barrier;
             barrier.image = m_entityID_color_attachment.get();
             barrier.srcStageBits = rhi::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
             barrier.srcMemoryAccessBits = 0;
             barrier.dstStageBits = rhi::PIPELINE_STAGE_TRANSFER_BIT;
             barrier.dstMemoryAccessBits = rhi::BARRIER_ACCESS_TRANSFER_READ_BIT;
             barrier.layoutBefore = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
             barrier.layoutAfter = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
             cmd->PipeLineBarriers(nullptr, 0, &barrier, 1, nullptr, 0);

             auto [mx, my] = ImGui::GetMousePos();
             mx -= m_viewportBounds[0].x;
             my -= m_viewportBounds[0].y;
             glm::vec2 viewportSize = m_viewportBounds[1] - m_viewportBounds[0];
             int mouseX = (int)mx;
             int mouseY = (int)my;
             uint32_t image_width = m_color_attachment->GetDesc().width;
             uint32_t image_height = m_color_attachment->GetDesc().height;
             int x = static_cast<int>(((mouseX / viewportSize.x) * image_width));
             int y = static_cast<int>(((mouseY / viewportSize.y) * image_height));
             cmd->CopyImageToBuffer(*m_stage_buffer, *m_entityID_color_attachment, 0, { x, y, 0 },
                 { 1, 1, 1 }, 0, 0, { rhi::ImageAspect::COLOR, 0, 0, 1 });

             rhi_device->SubmitCommandList(cmd, nullptr, 0, false);
         }

        rhi_device->EndFrame(ts);
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
        m_gizmoType = -1;
        break;
    case Key::W:
	    m_gizmoType = ImGuizmo::OPERATION::TRANSLATE;
		break;
    case Key::E:
        m_gizmoType = ImGuizmo::OPERATION::ROTATE;
        break;
    case Key::R:
		m_gizmoType = ImGuizmo::OPERATION::SCALE;
        break;
    default:
        break;
    }
}

void EditorApp::OnMouseButtonPressed(const MouseButtonPressedEvent& e)
{
    if (e.button == Mouse::ButtonLeft)
	{
		if (m_viewportHovered && !ImGuizmo::IsOver() && !Input::Get()->IsKeyPressed(Key::LeftAlt, true))
			m_heirarchyPanel.SetSelectedEntity(m_hoverdEntity);
	}
}

void EditorApp::CreateGraphicResources()
{
    using namespace quark::rhi;
    auto rhi_device = RenderSystem::Get().GetDevice();
    // Create depth image
    ImageDesc image_desc;
    image_desc.type = ImageType::TYPE_2D;
    auto ratio = Application::Get().GetWindow()->GetRatio();
    image_desc.width = uint32_t(Application::Get().GetWindow()->GetMonitorWidth() * Application::Get().GetWindow()->GetRatio());
    image_desc.height = uint32_t(Application::Get().GetWindow()->GetMonitorHeight() * Application::Get().GetWindow()->GetRatio());
    image_desc.depth = 1;
    image_desc.format = RenderSystem::Get().GetRenderResourceManager().format_depthAttachment_main;
    image_desc.arraySize = 1;
    image_desc.mipLevels = 1;
    image_desc.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    image_desc.usageBits = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    m_depth_attachment = rhi_device->CreateImage(image_desc);
    m_entityID_depth_attachment = rhi_device->CreateImage(image_desc);

    // Create color image
    image_desc.format = RenderSystem::Get().GetRenderResourceManager().format_colorAttachment_main;
    image_desc.initialLayout = ImageLayout::UNDEFINED;
    image_desc.usageBits = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rhi::IMAGE_USAGE_SAMPLING_BIT;
    m_color_attachment = rhi_device->CreateImage(image_desc);

    // Create entityID image
    image_desc.format = DataFormat::R32G32_UINT;
    image_desc.usageBits = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT;
    m_entityID_color_attachment = rhi_device->CreateImage(image_desc);

    // Create stage buffer
    BufferDesc buffer_desc;
    buffer_desc.domain = BufferMemoryDomain::CPU;
    buffer_desc.usageBits = BUFFER_USAGE_TRANSFER_TO_BIT;
    buffer_desc.size = image_desc.width * image_desc.height * sizeof(uint64_t);
    m_stage_buffer = rhi_device->CreateBuffer(buffer_desc);
}

}