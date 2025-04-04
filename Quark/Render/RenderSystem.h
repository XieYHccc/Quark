#pragma once
#include "Quark/Render/RenderResourceManger.h"
#include "Quark/RHI/Device.h"
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Core/Math/Frustum.h"

namespace quark {
class Scene;

struct RenderSystemConfig 
{
	uint32_t maxFramesInFlight = 2;
};

// 1. high level rendering api
// 2. a collection of graphics technique implentations and functions 
// to draw a scene, shadows, post processes and other things.
// 3. owner of the rhi device and render resource manager
class RenderSystem : public util::MakeSingleton<RenderSystem> {
public:
    RenderSystem(const RenderSystemConfig& config);
    ~RenderSystem();

    RenderResourceManager& GetRenderResourceManager() { return *m_renderResourceManager; }
    Ref<rhi::Device> GetDevice() { return m_device; }

    void BindCameraParameters(rhi::CommandList& cmd, const RenderContext& cxt);
    void BindLightingParameters(rhi::CommandList& cmd, const RenderContext& cxt);
    void Flush(rhi::CommandList& cmd, const RenderQueue& queue, const RenderContext& ctx);

    // update per frame buffer first then draw
    void DrawSkybox(uint64_t env_map_id, rhi::CommandList* cmd);
    void DrawGrid(rhi::CommandList* cmd);
    //void DrawScene(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd);
    //void DrawEntityID(const RenderScene& scene, const Visibility& vis, rhi::CommandList* cmd);
   
private:
    Ref<rhi::Device> m_device;
    Ref<rhi::Buffer> m_scene_ubo[10];
    uint32_t m_currentFrame = 0;
    Scope<RenderResourceManager> m_renderResourceManager;

};
}