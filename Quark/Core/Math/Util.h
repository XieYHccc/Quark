#pragma once
#include <cstdint>

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
}