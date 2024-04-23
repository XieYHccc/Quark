#include "./SceneMngr.h"

#include <iostream>

#include "AssestLoader/LoadGLTF.h"
#include "Graphics/Vulkan/RendererVulkan.h"

std::shared_ptr<Scene> SceneMngr::LoadGltfScene(std::filesystem::path filePath)
{
	auto newScene = loadGltf(filePath);
	sceneMap_.emplace(filePath, newScene);
	
	return newScene;
}

void SceneMngr::Finalize()
{
	vkDeviceWaitIdle(RendererVulkan::GetInstance()->GetVkDevice());
	
	for (auto [name, scene] : sceneMap_) {
		scene->ClearResourses();
	}
}