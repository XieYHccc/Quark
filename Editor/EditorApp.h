#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>
#include "Editor/UI/SceneHeirarchy.h"
#include "Editor/UI/Inspector.h"
#include "Editor/UI/SceneViewPort.h"

namespace editor {

class EditorApp : public Application  {  
public:
    EditorApp(const AppInitSpecs& specs);
    ~EditorApp();

    virtual void Update(f32 deltaTime) override final;
    virtual void Render(f32 deltaTime) override final;
    virtual void UpdateUI() override final;
    
    void CreateColorDepthAttachments();
    void CreatePipeline();
    void SetUpRenderPass();
    void LoadAsset();
    void SetUpCamera();

    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    graphic::RenderPassInfo geometry_pass_info; // First pass
    graphic::RenderPassInfo ui_pass_info;   // Second pass

    Ref<graphic::Image> depth_image;
    Ref<graphic::Image> color_image;
    graphic::DataFormat depth_format = graphic::DataFormat::D32_SFLOAT;
    graphic::DataFormat color_format;
    Scope<scene::Scene> scene_;
    Scope<render::SceneRenderer> scene_renderer_;

    // UI window
    ui::SceneHeirarchy heirarchyWindow_;
    ui::Inspector inspector_;
    ui::SceneViewPort sceneViewPort_;
    

    // Debug
    float cmdListRecordTime = 0;
private:
    void UpdateMainMenuUI();
};

}
