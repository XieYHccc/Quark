#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Core/FileSystem.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>
#include <Quark/Events/KeyEvent.h>

#include "Editor/EditorCamera.h"
#include "Editor/Panel/SceneHeirarchyPanel.h"
#include "Editor/Panel/InspectorPanel.h"
#include "Editor/Panel/ContentBrowserPanel.h"

namespace quark {
class EditorApp : public quark::Application {
public:
    EditorApp(const ApplicationSpecification& specs);
    ~EditorApp();

    void OnUpdate(TimeStep ts) override final;
    void OnRender(TimeStep ts) override final;
    void OnImGuiUpdate() override final;

    void OnKeyPressed(const KeyPressedEvent& e);

    void NewScene();
    void OpenScene();
    void OpenScene(const std::filesystem::path& path);
    void SaveSceneAs();

private:
    void CreateColorDepthAttachments();
    void CreateRenderPasses();

    graphic::RenderPassInfo forward_pass_info; // First pass
    graphic::RenderPassInfo ui_pass_info;   // Second pass

    Ref<graphic::Image> depth_attachment;
    Ref<graphic::Image> color_attachment;

    Ref<Texture> m_CubeMapTexture;

    Ref<Scene> m_Scene;
    Scope<SceneRenderer> m_SceneRenderer;
    EditorCamera m_EditorCamera;

    glm::vec2 m_ViewportSize;
    glm::vec2 m_ViewportBounds[2];

    bool m_ViewportFocused;
    bool m_ViewportHovered;
    int m_GizmoType = -1;

    // UI window
    SceneHeirarchyPanel m_HeirarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;

    // Debug
    float m_CmdListRecordTime = 0;
};

}