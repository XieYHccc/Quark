#pragma once
#include <glm/glm.hpp>

#include "Quark/Core/Math/Aabb.h"
#include "Quark/Core/Util/EnumCast.h"
#include "Quark/Asset/Material.h"

namespace quark {
//struct Vertex {
//    glm::vec3 position;
//    float uv_x;
//    glm::vec3 normal;
//    float uv_y;
//    glm::vec4 color;
//
//    bool operator==(const Vertex &other) const {
//        return position == other.position && color == other.color && normal == other.normal &&
//                uv_x == other.uv_x && uv_y == other.uv_y;
//    }
//};

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
};

class Mesh : public Asset {
public:
    // If isDynamic is true, These data will be freed after uploading to GPU
    // These attributes' lacation in shaders are fixed in Quark
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;

    std::vector<SubMeshDescriptor> subMeshes;
    //std::vector<Vertex> vertices;

    math::Aabb aabb = {};

    bool isDynamic = false;

public:
    Mesh() = default;
    //Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMeshDescriptor>& subMeshes, bool isDynamic = false);

    uint32_t GetMeshAttributeMask() const;

    void UpdateGpuBuffers(); // Carefully call this unless you know what you are doing
    void ReCalculateAabb();
    void ReCalculateNormals();
    void ReCalculateAabbs();

private:
    Ref<graphic::Buffer> GetVertexBuffer() const { return m_VertexBuffer; }
    Ref<graphic::Buffer> GetIndexBuffer() const { return m_IndexBuffer; }

    // Gpu resources
    Ref<graphic::Buffer> m_VertexBuffer;
    Ref<graphic::Buffer> m_IndexBuffer;
    
    // Overlapped vertex data. will be freed after uploading to GPU if this is a static mesh
    std::vector<uint8_t> m_OverlappedVertexData;

    friend class SceneRenderer;

};
}