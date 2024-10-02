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

struct CameraUniformBufferBlock 
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct SceneUniformBufferBlock 
{
    CameraUniformBufferBlock cameraUboData;

    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct ModelPushConstants 
{
    // Per geometry
    glm::mat4 worldMatrix = glm::mat4(1.f);
    // u64 vertexBufferGpuAddress; // deprecated currently
};

struct MaterialPushConstants
{
    // Per material
    glm::vec4 colorFactors = glm::vec4(1.f);
    float metallicFactor = 1.f;
    float roughnessFactor = 1.f;
};

// The minimum unit for a single draw call
struct RenderObject 
{
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    Ref<graphic::Buffer> indexBuffer;
    Ref<graphic::Buffer> attributeBuffer;
    Ref<graphic::Buffer> positionBuffer;
    Ref<graphic::PipeLine> pipeLine;
    Ref<Material> material;
    math::Aabb aabb = {};
    glm::mat4 transform;

    //Editor
    uint64_t entityID = 0;
};

class SceneRenderer {
public: 
    SceneRenderer(graphic::Device* device);

    void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }
    void SetCubeMap(const Ref<Texture>& cubeMap) { m_CubeMap = cubeMap; }

    void DrawScene(graphic::CommandList* cmd_list);
    void DrawSkybox(graphic::CommandList* cmd_list);

    void UpdateDrawContext();
    void UpdateDrawContext(const CameraUniformBufferBlock& cameraData); // Update scene uniform buffer with custom camera data(Used in Editor now)

private:
    void UpdateRenderObjects();

    graphic::Device* m_GraphicDevice;

    Ref<Scene> m_Scene;
    Ref<Texture> m_CubeMap;

    // Data need to be updated every frame
    struct DrawContext 
    {
        std::vector<RenderObject> opaqueObjects;
        std::vector<RenderObject> transparentObjects;
        std::vector<uint32_t> opaqueDraws;   // indices point to  opaque render objects vector
        std::vector<uint32_t> transparentDraws; // indices point to transparent render objects vector
        SceneUniformBufferBlock sceneUboData;
        Ref<graphic::Buffer> sceneUniformBuffer;
        math::Frustum frustum;
    } m_DrawContext;
};

}