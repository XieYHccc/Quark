#pragma once
#include "Quark/Render/RenderTypes.h"
#include "Quark/Render/RenderResourceManger.h"
#include "Quark/Render/RenderSwapContext.h"
#include "Quark/RHI/Device.h"
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Core/Math/Frustum.h"


namespace quark {
class Scene;

// 1. high level rendering api
// 2. a collection of graphics technique implentations and functions 
// to draw a scene, shadows, post processes and other things.
// 3. owner of the render resource manager
class RenderSystem : public util::MakeSingleton<RenderSystem> {
public:
    struct DrawContext 
    {
        std::vector<RenderObject> objects_opaque;
        std::vector<RenderObject> objects_transparent;

        UniformBufferData_Scene sceneData;

        // gpu resources
        Ref<rhi::Buffer> sceneUB;
    };

    struct Visibility
    {
        UniformBufferData_Camera cameraData;
        math::Frustum frustum;
        std::vector<uint32_t> visible_opaque;
        std::vector<uint32_t> visible_transparent;
    };

public:
    RenderSystem(Ref<rhi::Device> device);
    ~RenderSystem();

    RenderResourceManager& GetRenderResourceManager() { return *m_renderResourceManager; }
    RenderSwapContext& GetSwapContext() { return m_swapContext; }

    void ProcessSwapData();

    // these two function is used to sync rendering data with the scene
    void UpdateDrawContext(const Ref<Scene>& scene, DrawContext& context);
    void UpdateVisibility(const DrawContext& context, Visibility& vis, const UniformBufferData_Camera& cameraData);
    
    void UpdateGpuResources(DrawContext& context, Visibility& vis);

    // update per frame buffer first then draw
    void DrawSkybox(uint64_t env_map_id, rhi::CommandList* cmd);
    void DrawGrid(rhi::CommandList* cmd);
    void DrawScene(const RenderScene& scene, const RenderScene::Visibility& vis, rhi::CommandList* cmd);
    void DrawEntityID(const RenderScene& scene, const RenderScene::Visibility& vis, rhi::CommandList* cmd);
    // old api
    void DrawSkybox(const DrawContext& context, const Ref<Texture>& envMap, rhi::CommandList* cmd);
    void DrawScene(const DrawContext& context, const Visibility& vis, rhi::CommandList* cmd);
    void DrawGrid(const DrawContext& context, rhi::CommandList* cmd);
    void DrawEntityID(const DrawContext& context, const Visibility& vis, rhi::CommandList* cmd);

private:
    Ref<rhi::Device> m_device;

    Scope<RenderResourceManager> m_renderResourceManager;

    RenderSwapContext m_swapContext;

    Ref<RenderScene> m_renderScene;

};
}