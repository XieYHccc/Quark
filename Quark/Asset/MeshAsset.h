#pragma once
#include "Quark/Asset/Asset.h"
#include "Quark/Core/Math/Aabb.h"
#include "Quark/Core/Util/EnumCast.h"
#include <glm/glm.hpp>

namespace quark {
struct MaterialAsset;

enum class MeshAttribute : unsigned
{
    POSITION = 0,
    UV = 1,
    NORMAL = 2,
    TANGENT = 3,
    BONE_INDEX = 4,
    BONE_WIGHTS = 5,
    VERTEX_COLOR = 6,
    MAX_ENUM,
};

enum MeshAttributeFlagBits
{
    MESH_ATTRIBUTE_POSITION_BIT = 1u << util::ecast(MeshAttribute::POSITION),
    MESH_ATTRIBUTE_UV_BIT = 1u << util::ecast(MeshAttribute::UV),
    MESH_ATTRIBUTE_NORMAL_BIT = 1u << util::ecast(MeshAttribute::NORMAL),
    MESH_ATTRIBUTE_TANGENT_BIT = 1u << util::ecast(MeshAttribute::TANGENT),
    MESH_ATTRIBUTE_BONE_INDEX_BIT = 1u << util::ecast(MeshAttribute::BONE_INDEX),
    MESH_ATTRIBUTE_BONE_WEIGHT_BIT = 1u << util::ecast(MeshAttribute::BONE_WIGHTS),
    MESH_ATTRIBUTE_VERTEX_COLOR_BIT = 1u << util::ecast(MeshAttribute::VERTEX_COLOR)
};

class MeshAsset : public Asset {
public:
    QUARK_ASSET_TYPE_DECL(MESH)
    struct SubMeshDescriptor 
    {
        uint32_t startVertex = 0;
        uint32_t startIndex = 0; // This is not relative to the startVertex
        uint32_t count = 0;
        math::Aabb aabb = {};
        AssetID materialID = 0;
    };
    std::vector<SubMeshDescriptor> subMeshes;

    std::vector<uint32_t> indices;
    std::vector<glm::vec3> vertex_positions;
    std::vector<glm::vec2> vertex_uvs;
    std::vector<glm::vec3> vertex_normals;
    std::vector<glm::vec3> vertex_tangents;
    std::vector<glm::vec4> vertex_colors;
    std::vector<glm::ivec4> vertex_bone_indices;
    std::vector<glm::vec4> vertex_bone_weights;

    math::Aabb aabb = {};

public:
    MeshAsset() = default;

    uint32_t GetMeshAttributeMask() const;
    size_t GetVertexCount() const { return vertex_positions.size(); }
    size_t GetPositionBufferStride() const;
    size_t GetAttributeBufferStride() const;

    void SetDynamic(bool isDynamic) { this->m_isDynamic = isDynamic; }
    bool IsDynamic() const { return m_isDynamic; }

    void CalculateAabbs();
    void CalculateNormals();

    bool IsVertexDataArraysValid() const;

private:
    bool m_isDynamic = false;

};
}