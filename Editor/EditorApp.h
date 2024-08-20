#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>
#include <Quark/Events/KeyEvent.h>

#include "Editor/EditorCamera.h"
#include "Editor/Panel/SceneHeirarchyPanel.h"
#include "Editor/Panel/InspectorPanel.h"
#include "Editor/Panel/ContentBrowserPanel.h"

namespace quark {
class EditorApp : public quark::Application  {  
public:
    EditorApp(const quark::AppInitSpecs& specs);
    ~EditorApp();

    void OnUpdate(TimeStep ts) override final;
    void OnRender(TimeStep ts) override final;
    void OnImGuiUpdate() override final;

    void OnKeyPressed(const KeyPressedEvent& e);

    void NewScene();
    void OpenScene();
    void SaveSceneAs();

    void CreateColorDepthAttachments();
    void CreatePipeline();
    void SetUpRenderPass();
    void LoadScene();

public:
    Ref<quark::graphic::Shader> vert_shader;
    Ref<quark::graphic::Shader> frag_shader;
    Ref<quark::graphic::Shader> skybox_vert_shader;
    Ref<quark::graphic::Shader> skybox_frag_shader;
    
    Ref<quark::graphic::PipeLine> graphic_pipeline;
    Ref<quark::graphic::PipeLine> skybox_pipeline;

    graphic::RenderPassInfo forward_pass_info; // First pass
    graphic::RenderPassInfo ui_pass_info;   // Second pass

    Ref<quark::graphic::Sampler> m_DefaultLinearSampler;
    Ref<quark::graphic::Image> cubeMap_image;
    Ref<quark::graphic::Image> depth_image;
    Ref<quark::graphic::Image> color_image;
    graphic::DataFormat depth_format = quark::graphic::DataFormat::D32_SFLOAT;
    graphic::DataFormat color_format; // Same with swapchain format
    
    Scope<Scene> m_Scene;
    Scope<SceneRenderer> m_SceneRenderer;
    EditorCamera m_EditorCamera;

    bool m_ViewportFocused, m_ViewportHovered;

    glm::vec2 m_ViewportSize;
    glm::vec2 m_ViewportBounds[2];

    ImTextureID m_ColorAttachmentId;

    int m_GizmoType = -1;

    // UI window
    SceneHeirarchyPanel m_HeirarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    
    // Debug
    float cmdListRecordTime = 0;
};

}