#pragma once
#include <glm/glm.hpp>

#include "Quark/Core/Math/Aabb.h"
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Asset/Material.h"

namespace quark {
struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;

    bool operator==(const Vertex &other) const {
        return position == other.position && color == other.color && normal == other.normal &&
                uv_x == other.uv_x && uv_y == other.uv_y;
    }
};

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
    NONE
};

enum MeshAttributeFlagBits
{
    MESH_ATTRIBUTE_POSITION_BIT = 1u << util::ecast(MeshAttribute::POSITION),
    MESH_ATTRIBUTE_UV_BIT = 1u << util::ecast(MeshAttribute::UV),
    MESH_ATTRIBUTE_NORMAL_BIT = 1u << util::ecast(MeshAttribute::NORMAL),
    MESH_ATTRIBUTE_TANGENT_BIT = 1u << util::ecast(MeshAttribute::TANGENT),
    MESH_ATTRIBUTE_BONE_INDEX_BIT = 1u << util::ecast(MeshAttribute::BONE_INDEX),
    MESH_ATTRIBUTE_BONE_WEIGHTS_BIT = 1u << util::ecast(MeshAttribute::BONE_WIGHTS),
    MESH_ATTRIBUTE_VERTEX_COLOR_BIT = 1u << util::ecast(MeshAttribute::VERTEX_COLOR)
};

struct SubMeshDescriptor {
    uint32_t startIndex = 0;
    uint32_t count = 0;
    math::Aabb aabb = {};
    Ref<Material> material = nullptr;
};

class Mesh : public Asset {
public:
    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMeshDescriptor>& subMeshes, bool isDynamic = false);

    void CreateRenderResources();
    void ReCalculateNormals();
    void ReCalculateAabbs();
    
    std::vector<SubMeshDescriptor> subMeshes;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    math::Aabb aabb = {};
    bool isDynamic = false;

    // Gpu resources
    Ref<graphic::Buffer> vertexBuffer;
    Ref<graphic::Buffer> attributeBuffer;
    Ref<graphic::Buffer> indexBuffer;

private:
    std::vector<uint32_t> m_Indices;

    std::vector<glm::vec3> m_Positions;
    std::vector<glm::vec2> m_UVs;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec4> m_Colors;

};
}