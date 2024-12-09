#pragma once
#include "Quark/Core/Math/Aabb.h"
#include "Quark/RHI/Common.h"
#include "Quark/Asset/Material.h"

#include <glm/glm.hpp>

namespace quark {

struct UniformBufferData_Camera
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

struct UniformBufferData_Scene
{
    UniformBufferData_Camera cameraUboData;

    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct PushConstants_Model
{
    // Per geometry
    glm::mat4 worldMatrix = glm::mat4(1.f);
    // u64 vertexBufferGpuAddress; // deprecated currently
};

struct PushConstants_Material
{
    // Per material
    glm::vec4 colorFactors = glm::vec4(1.f);
    float metallicFactor = 1.f;
    float roughnessFactor = 1.f;
};

struct RenderMesh 
{
    Ref<rhi::Buffer> vertex_position_buffer;
    Ref<rhi::Buffer> vertex_varying_buffer;
    Ref<rhi::Buffer> index_buffer;

    uint32_t vertex_count;
    uint32_t index_count;
};

struct RenderPBRMaterial 
{
    Ref<rhi::Image> base_color_texture_image;
    Ref<rhi::Image> normal_texture_image;
    Ref<rhi::Image> metallic_roughness_texture_image;
    Ref<rhi::Image> emissive_texture_image;

    PushConstants_Material material_push_constants;
};

struct RenderEntity
{
    uint32_t instanceId;
    glm::mat4 modelMatrix;

    // mesh
    uint32_t gpu_mesh_id;
    math::Aabb boundingBox;

    // material
    uint32_t gpu_material_id;

};

// The minimum unit for a single draw call
struct RenderObject
{
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    Ref<rhi::Buffer> indexBuffer;
    Ref<rhi::Buffer> attributeBuffer;
    Ref<rhi::Buffer> positionBuffer;
    Ref<rhi::PipeLine> mainPassPipeLine;
    Ref<Material> material;
    math::Aabb aabb = {};
    glm::mat4 transform;

    //Editor
    uint64_t entityID = 0;
};
}