#pragma once
#include <glm/glm.hpp>

#include "Quark/Core/Math/Aabb.h"
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
    Ref<graphic::Buffer> indexBuffer;
};
}