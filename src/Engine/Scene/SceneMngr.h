#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "GameObject/GameObject.h"
#include "Scene/Scene.h"

class SceneMngr {
public:
	static SceneMngr& Instance() {
		static SceneMngr instance;
		return instance;
	}
	void Initialize() {};
	void Finalize();

	std::shared_ptr<Scene> LoadGltfScene(std::filesystem::path filePath);

private:
	SceneMngr() {}

	std::unordered_map<std::string, std::shared_ptr<Scene>>sceneMap_;


};