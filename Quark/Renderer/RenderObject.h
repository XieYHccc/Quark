#pragma once
#include "Quark/Core/Math/Aabb.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Asset/Material.h"

namespace quark {

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