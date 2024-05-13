#pragma once
#include "Scene/Scene.h"

std::unique_ptr<Scene> loadGltf(const std::filesystem::path& filePath);
