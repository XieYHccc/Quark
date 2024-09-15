#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace quark::math {
constexpr uint32_t GetNextPowerOfTwo(uint32_t x)
{
    if (x == 0) {
        return 1;
    }

    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}
constexpr uint64_t GetNextPowerOfTwo(uint64_t x)
{
    if (x == 0) {
        return 1;
    }
    
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32u;
    return ++x;
}

bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

}