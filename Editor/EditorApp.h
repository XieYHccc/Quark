#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>
#include "Editor/UI/SceneHeirarchy.h"
#include "Editor/UI/Inspector.h"
#include "Editor/UI/SceneViewPort.h"

namespace quark {
class EditorApp : public quark::Application  {  
public:
    EditorApp(const quark::AppInitSpecs& specs);
    ~EditorApp();

    void Update(float deltaTime) override final;
    void Render(float deltaTime) override final;
    void UpdateUI() override final;
    
    void CreateColorDepthAttachments();
    void CreatePipeline();
    void SetUpRenderPass();
    void LoadScene();
    void UpdateMainMenuUI();

    quark::Ref<quark::graphic::Shader> vert_shader;
    quark::Ref<quark::graphic::Shader> frag_shader;
    quark::Ref<quark::graphic::Shader> skybox_vert_shader;
    quark::Ref<quark::graphic::Shader> skybox_frag_shader;
    
    quark::Ref<quark::graphic::PipeLine> graphic_pipeline;
    quark::Ref<quark::graphic::PipeLine> skybox_pipeline;

    quark::graphic::RenderPassInfo forward_pass_info; // First pass
    quark::graphic::RenderPassInfo ui_pass_info;   // Second pass

    quark::Ref<quark::graphic::Image> cubeMap_image;
    quark::Ref<quark::graphic::Image> depth_image;
    quark::Ref<quark::graphic::Image> color_image;
    quark::graphic::DataFormat depth_format = quark::graphic::DataFormat::D32_SFLOAT;
    quark::graphic::DataFormat color_format; // Same with swapchain format
    
    quark::Scope<quark::Scene> m_Scene;
    quark::Scope<quark::SceneRenderer> m_SceneRenderer;
    Entity* m_EditorCameraEntity = nullptr;

    // UI window
    SceneHeirarchy heirarchyWindow_;
    Inspector inspector_;
    SceneViewPort sceneViewPort_;
    
    // Debug
    float cmdListRecordTime = 0;
};

}