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
    Ref<rhi::Buffer> vertex_varying_enable_blending_buffer; // normal, tangent..
    Ref<rhi::Buffer> vertex_varying_buffer; // uv, color...
    Ref<rhi::Buffer> vertex_joint_binding_buffer;
    Ref<rhi::Buffer> index_buffer;

    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t mesh_attribute_mask;

    bool isDynamic = false;
};

struct RenderPBRMaterial 
{
    Ref<rhi::Image> base_color_texture_image;
    Ref<rhi::Image> normal_texture_image;
    Ref<rhi::Image> metallic_roughness_texture_image;
    Ref<rhi::Image> emissive_texture_image;

    PushConstants_Material material_push_constants;
};

struct RenderObject1
{
    uint64_t id;
    glm::mat4 model_matrix;

    // mesh
    uint64_t render_mesh_id;
    uint32_t start_index;
    uint32_t index_count;
    math::Aabb bounding_box;

    // material
    uint64_t render_material_id;

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