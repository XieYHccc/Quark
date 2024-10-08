#pragma once
#include <glm/glm.hpp>
#include "Quark/Core/Math/Frustum.h"
#include "Quark/Asset/Material.h"
#include "Quark/Asset/Mesh.h"
#include "Quark/Renderer/RenderObject.h"
namespace quark {

namespace graphic 
{
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

struct DrawContext
{
    std::vector<RenderObject> opaqueObjects;
    std::vector<RenderObject> transparentObjects;
    std::vector<uint32_t> opaqueDraws;   // indices point to  opaque render objects vector
    std::vector<uint32_t> transparentDraws; // indices point to transparent render objects vector
    Ref<graphic::Buffer> sceneUniformBuffer;
    math::Frustum frustum;
};

class SceneRenderer {
public: 
    SceneRenderer(graphic::Device* device);

    void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }

    void DrawScene(graphic::CommandList* cmd_list);
    void DrawSkybox(const Ref<Texture>& envMap, graphic::CommandList* cmd_list);

    void UpdateDrawContext();
    void UpdateDrawContextEditor(const CameraUniformBufferBlock& cameraData); // Update scene uniform buffer with custom camera data(Used in Editor now)

private:
    void UpdateRenderObjects();

    // encapsule all needed data for rendering a single frame
    DrawContext m_DrawContext;

    graphic::Device* m_GraphicDevice;

    Ref<Scene> m_Scene;
};

}