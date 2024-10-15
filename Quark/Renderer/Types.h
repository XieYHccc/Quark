#pragma once
#include "Quark/Core/Math/Aabb.h"
#include "Quark/Graphic/Common.h"
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

// The minimum unit for a single draw call
struct RenderObject
{
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    Ref<graphic::Buffer> indexBuffer;
    Ref<graphic::Buffer> attributeBuffer;
    Ref<graphic::Buffer> positionBuffer;
    Ref<graphic::PipeLine> pipeLine;
    Ref<Material> material;
    math::Aabb aabb = {};
    glm::mat4 transform;

    //Editor
    uint64_t entityID = 0;
};
}