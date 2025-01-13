#include "Quark/Asset/MeshAsset.h"
#include "Quark/Core/Application.h"

namespace quark {


uint32_t MeshAsset::GetMeshAttributeMask() const
{
    uint32_t result = 0;

    if (!vertex_positions.empty())
		result |= MESH_ATTRIBUTE_POSITION_BIT;
    if (!vertex_uvs.empty())
        result |= MESH_ATTRIBUTE_UV_BIT;
    if (!vertex_normals.empty())
        result |= MESH_ATTRIBUTE_NORMAL_BIT;
    if (!vertex_colors.empty())
        result |= MESH_ATTRIBUTE_VERTEX_COLOR_BIT;
    if (!vertex_bone_indices.empty())
        result |= MESH_ATTRIBUTE_BONE_INDEX_BIT;
    if (!vertex_bone_weights.empty())
        result |= MESH_ATTRIBUTE_BONE_WEIGHT_BIT;

    return result;
}

size_t MeshAsset::GetPositionBufferStride() const
{
    return sizeof(decltype(vertex_positions)::value_type);
}

size_t MeshAsset::GetAttributeBufferStride() const
{
    size_t stride = 0;

    if (!vertex_uvs.empty()) stride += sizeof(decltype(vertex_uvs)::value_type);
    if (!vertex_normals.empty()) stride += sizeof(decltype(vertex_normals)::value_type);
    if (!vertex_colors.empty()) stride += sizeof(decltype(vertex_colors)::value_type);

    return stride;
}

bool MeshAsset::IsVertexDataArraysValid() const
{
    size_t num = vertex_positions.size();
    if (num != 0
        && (vertex_uvs.size() == 0 || vertex_uvs.size() == num)
        && (vertex_normals.size() == 0 || vertex_normals.size() == num)
        && (vertex_colors.size() == 0 || vertex_colors.size() == num) )
        return true;
    else
        return false;
}

void MeshAsset::CalculateAabbs()
{
    if (vertex_positions.empty())
        return;

    for (auto& submesh : this->subMeshes)
    {
        submesh.aabb = {};
        for (size_t i = 0; i < submesh.count; i++) 
        {
            glm::vec3 position = vertex_positions[indices[submesh.startIndex + i]];
            submesh.aabb += position;
        }

        this->aabb += submesh.aabb;

        QK_CORE_ASSERT(submesh.aabb.Min() != submesh.aabb.Max())
    }
}

}