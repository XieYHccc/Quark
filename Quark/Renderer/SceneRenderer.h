#pragma once
#include <glm/glm.hpp>
#include "Renderer/RenderTypes.h"
#include "Math/Frustum.h"

namespace scene {
class Scene;
}

namespace graphic {
class CommandList;
class Device;
}

namespace render {
class SceneRenderer {
public:
    struct GpuDrawPushConstants {
        // Per geometry
        glm::mat4 worldMatrix = glm::mat4(1.f);
        u64 vertexBufferGpuAddress;

        // Per material
        // float metallicFactor = 1.f;
        // float roughnessFactor = 1.f;
        // glm::vec4 ColorFactors = glm::vec4(1.f);
    };

    struct SceneUniformBufferBlock {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection; // w for sun power
        glm::vec4 sunlightColor;
    };

    struct RenderObject {
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        graphic::Buffer* indexBuffer = nullptr;
        graphic::Buffer* vertexBuffer = nullptr;
        Material* material = nullptr;
        math::Aabb aabb = {};
        glm::mat4 transform;
    };

public: 
    SceneRenderer(graphic::Device* device);

    void SetScene(scene::Scene* scene);
    void SetCubeMap(Ref<graphic::Image> cubeMap) { cubeMap_ = cubeMap; }
    void Render(graphic::CommandList* cmd_list);
    void RenderSkybox(graphic::CommandList* cmd_list);
    void UpdateDrawContext();
private:
    void PrepareForRender();
    
    graphic::Device* device_;
    scene::Scene* scene_;
    Ref<graphic::Image> cubeMap_;
    Ref<graphic::Sampler> cubeMapSampler_;
    Ref<render::Mesh> cubeMesh_;

    // Data need to be updated every frame
    struct DrawContext {
        std::vector<RenderObject> opaqueObjects;
        std::vector<RenderObject> transparentObjects;
        SceneUniformBufferBlock sceneData;
        std::vector<u32> opaqueDraws;   // indices point to  opaque render objects vector
        std::vector<u32> transparentDraws; // indices point to transparent render objects vector
        Ref<graphic::Buffer> sceneUniformBuffer;
        math::Frustum frustum_;
    } drawContext_;
};
}