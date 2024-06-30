#pragma once
#include <glm/glm.hpp>
#include "Graphic/Common.h"

namespace render {

struct Material;

struct Bounds {
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;
};

struct SubMeshDescriptor {
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    Ref<Material> material;
};

struct Mesh {
    Ref<graphic::Buffer> vertexBuffer;
    Ref<graphic::Buffer> indexBuffer;
    size_t vertexBufferOffset = 0;
    size_t indexBufferOffset = 0;
    size_t vertexCount = 0;
    std::vector<SubMeshDescriptor> submeshes;
};
}