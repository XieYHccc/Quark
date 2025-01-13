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
    RenderSystem(Ref<rhi::Device> device);
    ~RenderSystem();

    RenderResourceManager& GetRenderResourceManager() { return *m_renderResourceManager; }
    RenderSwapContext& GetSwapContext() { return m_swapContext; }
    Ref<RenderScene> GetRenderScene() { return m_renderScene; }

    void ProcessSwapData();

    // update per frame buffer first then draw
    void DrawSkybox(uint64_t env_map_id, rhi::CommandList* cmd);
    void DrawGrid(rhi::CommandList* cmd);
    void DrawScene(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd);
    void DrawEntityID(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd);
   
private:
    Ref<rhi::Device> m_device;
    Scope<RenderResourceManager> m_renderResourceManager;
    RenderSwapContext m_swapContext;
    Ref<RenderScene> m_renderScene;

};
}