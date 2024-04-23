#pragma once

//#define GRAPHIC_API_OPENGL
#define GRAPHIC_API_VULKAN

#ifdef GRAPHIC_API_VULKAN
#define GLFW_INCLUDE_NONE
#endif

#include <algorithm>
#include <any>
#include <bitset>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <variant>
#include <array>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "Foundation/Log/Logger.h"
#include "Foundation/Debug.h"