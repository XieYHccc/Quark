#pragma once
#include "pch.h"
#include "Scene/Scene.h"

std::shared_ptr<Scene> loadGltf(std::filesystem::path filePath);
