#pragma once
#include <glm/glm.hpp>
#include "Quark/Core/Math/Frustum.h"
#include "Quark/Asset/Material.h"
#include "Quark/Asset/Mesh.h"

namespace quark {

namespace graphic {
class CommandList;
class Device;
}

class Scene;

struct CameraUniformBufferBlock {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct SceneUniformBufferBlock {
    CameraUniformBufferBlock cameraData;

    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct GpuDrawPushConstants {
    // Per geometry
    glm::mat4 worldMatrix = glm::mat4(1.f);
    u64 vertexBufferGpuAddress;

    // Per material
    // float metallicFactor = 1.f;
    // float roughnessFactor = 1.f;
    // glm::vec4 ColorFactors = glm::vec4(1.f);
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

class SceneRenderer {
public: 
    SceneRenderer(graphic::Device* device);

    void SetScene(Scene* scene);
    void SetCubeMap(const Ref<Texture>& cubeMap) { m_CubeMap = cubeMap; }
    void RenderScene(graphic::CommandList* cmd_list);
    void RenderSkybox(graphic::CommandList* cmd_list);
    void UpdateDrawContext();
    void UpdateDrawContext(const CameraUniformBufferBlock& cameraData); // Update scene uniform buffer with custom camera data(Used in Editor now)

private:
    void PrepareForRender();
    void UpdateRenderObjects();

    graphic::Device* m_GraphicDevice;
    Scene* m_Scene;
    Ref<Texture> m_CubeMap;
    Ref<Mesh> m_CubeMesh;

    // Data need to be updated every frame
    struct DrawContext {
        std::vector<RenderObject> opaqueObjects;
        std::vector<RenderObject> transparentObjects;
        SceneUniformBufferBlock sceneData;
        std::vector<u32> opaqueDraws;   // indices point to  opaque render objects vector
        std::vector<u32> transparentDraws; // indices point to transparent render objects vector
        Ref<graphic::Buffer> sceneUniformBuffer;
        math::Frustum frustum;
    } m_DrawContext;
};

}