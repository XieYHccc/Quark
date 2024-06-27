#pragma once

#include <filesystem>
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
	void Init() {};
	void Finalize();

	Scene* LoadGLTFScene(const std::filesystem::path& filePath);
	Scene* CreateScene(const std::string& name);

private:
	SceneMngr() {}

	std::unordered_map<std::string, std::unique_ptr<Scene>>sceneMap_;
	


};
