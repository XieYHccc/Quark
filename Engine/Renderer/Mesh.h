#pragma once
#include <glm/glm.hpp>

#include "Asset/BaseAsset.h"
#include "Graphics/Vulkan/Buffer.h"
#include "Renderer/Material.h"
#include "Renderer/Bounds.h"

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
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<asset::Material> material;
};

namespace asset {

struct MeshCreateInfo
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMeshDescriptor> submeshs;
    bool dynamic { false };

    std::string path;
    std::string name;
};

class Mesh : public BaseAsset  {
    static std::unordered_map<std::string, std::shared_ptr<Mesh>> assetPool_;
public: 
    static std::shared_ptr<Mesh> AddToPool(const MeshCreateInfo& createInfo);
    static void ClearPool() { assetPool_.clear();}

    Mesh(const MeshCreateInfo& createInfo);
    ~Mesh();

    ASSET_TYPE("Renderer::Mesh")

    inline vk::Buffer GetVertexBuffer() { return meshBuffers_.vertexBuffer; }
    inline vk::Buffer GetIndexBuffer() { return meshBuffers_.indexBuffer; }
    inline VkDeviceAddress GetVertBufferAddress() { return meshBuffers_.vertexBufferAddress; }

    inline bool IsDynamic() { return dynamic_; }

    inline const std::vector<SubMeshDescriptor> GetSubMeshes() const { return submeshs_; }

private:
    struct MeshBuffers
    {
        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    } meshBuffers_;

    std::vector<SubMeshDescriptor> submeshs_;
    bool dynamic_;

};  

}