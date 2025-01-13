#pragma once
#include "Quark/Core/Math/Aabb.h"
#include "Quark/RHI/Common.h"
#include "Quark/Render/ShaderLibrary.h"
#include "Quark/Asset/MaterialAsset.h"
#include <glm/glm.hpp>

namespace quark {

    static const uint32_t s_mesh_vertex_blending_max_joint_count = 128;

    struct UniformBufferObject_Camera
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
    };

    struct UniformBufferObject_Scene
    {
        UniformBufferObject_Camera cameraUboData;

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

    struct PerDrawCall_StorageBufferObject_EnableMeshVertexBlending
    {
        glm::mat4 joint_matrices[s_mesh_vertex_blending_max_joint_count];
    };

    struct RenderMesh 
    {
        Ref<rhi::Buffer> vertex_position_buffer;
        Ref<rhi::Buffer> vertex_varying_enable_blending_buffer; // normal, tangent..
        Ref<rhi::Buffer> vertex_varying_buffer; // uv, color...
        Ref<rhi::Buffer> vertex_joint_binding_buffer; // for skinned mesh
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

        glm::vec4 colorFactors = glm::vec4(1.f);
        float metallicFactor = 1.f;
        float roughnessFactor = 1.f;

        ShaderProgram* shaderProgram;
        AlphaMode alphaMode;
    };

    // minimum unit for a single draw call
    struct RenderObject
    {
        uint64_t id = 0;
        glm::mat4 model_matrix;

        // mesh
        uint64_t render_mesh_id = 0;
        uint32_t start_index = 0;
        uint32_t index_count = 0;
        math::Aabb aabb;
        bool enable_vertex_blending = false;
        std::vector<glm::mat4> joint_matrices;
        // TODO: cache this or use a global ssbo to store all joint matrices
        Ref<rhi::Buffer> perdrawcall_ssbo_mesh_joint_matrices = nullptr;

        // material
        uint64_t render_material_id = 0;
    };
}