#include "./SceneMngr.h"

#include <iostream>

#include "Asset/LoadGLTF.h"
#include "Renderer/Renderer.h"

Scene* SceneMngr::LoadGLTFScene(const std::filesystem::path& filePath)
{
	auto newScene = loadGltf(filePath);
	Scene* raw = newScene.get();

	sceneMap_.emplace(newScene->name, std::move(newScene));
	
	return raw;
}

Scene* SceneMngr::CreateScene(const std::string &name)
{
	auto newScene = std::make_unique<Scene>(name);
	Scene* raw = newScene.get();

	sceneMap_.emplace(name, std::move(newScene));

	CORE_LOGD("Create scene \"{}\"", name)
	
	return raw;
}

void SceneMngr::Finalize()
{	
	for (auto& [name, scene] : sceneMap_) {
		scene.reset();
	}
}