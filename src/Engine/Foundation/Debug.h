#pragma once
#include <cassert>

#define XE_DYNAMIC_ASSERT(condition) assert(condition);
#define XE_STATIC_ASSERT(condition) static_assert(condition);
