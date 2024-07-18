#pragma once
#include <glm/glm.hpp>
#include "Math/Aabb.h"
#include "Graphic/Common.h"

namespace render {

class Resource {
public:
    Resource() = default;
    virtual ~Resource() = default;

    void SetName(const std::string& name) { name_ = name; }
    std::string GetName() const { return name_; }
private:
    std::string name_;
};

struct Texture : public Resource{
    Ref<graphic::Image> image;
    Ref<graphic::Sampler> sampler;
};

struct Material : public Resource{
    enum class AlphaMode {
        OPAQUE,
        TRANSPARENT
    };

    struct UniformBufferBlock {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        float metalicFactor = 1.f;
        float roughNessFactor = 1.f;
    } uniformBufferData;

    AlphaMode alphaMode = AlphaMode::OPAQUE;
    Ref<graphic::Buffer> uniformBuffer = nullptr;
    size_t uniformBufferOffset = 0;
    Ref<Texture> baseColorTexture;
    Ref<Texture> metallicRoughnessTexture;
    Ref<Texture> normalTexture;
};

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

struct Mesh : public Resource {
    struct SubMeshDescriptor {
        uint32_t startIndex = 0;
        uint32_t count = 0;
        math::Aabb aabb = {};
        Ref<Material> material = nullptr;
    };
    std::vector<SubMeshDescriptor> subMeshes;
    
    Ref<graphic::Buffer> vertexBuffer = nullptr;
    Ref<graphic::Buffer> indexBuffer = nullptr;
    size_t vertexCount = 0;
    size_t indexCount = 0;
    math::Aabb aabb = {};

    Mesh(const std::vector<Vertex>& vertices, const std::vector<u32> indices, const std::vector<SubMeshDescriptor>& submeshes);
};
}