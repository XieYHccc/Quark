#include "Scene/Scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "GameObject/Components/MeshRendererCmpt/MeshRenderCmpt.h"
#include "Graphics/Vulkan/RendererVulkan.h"
#include "Application/Application.h"

Scene::Scene()
{
	root_ = std::make_unique<GameObject>("root");

	drawContext_.sceneData.ambientColor = glm::vec4(.1f);
	drawContext_.sceneData.sunlightColor = glm::vec4(1.f);
	drawContext_.sceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);
}

GameObject* Scene::AddGameObject(const std::string& name, GameObject* parent) {
	std::unique_ptr<GameObject> newObject = std::make_unique<GameObject>(name, parent);

	if (parent != nullptr) {
		newObject->parent = parent;
		parent->childrens.push_back(newObject.get());
	}
	else {
		newObject->parent = root_.get();
		root_->childrens.push_back(newObject.get());
	}

	// move ownership
	gameObjects_.push_back(std::move(newObject));

	return gameObjects_.back().get();
}

void Scene::Update()
{
	drawContext_.OpaqueObjects.clear();
	drawContext_.TransparentObjects.clear();

	// update render objects
	for (const auto& gameobject : gameObjects_) {
		auto meshRenderer = gameobject->GetComponent<MeshRendererCmpt>();
		if (meshRenderer != nullptr) {
			for (const auto& renderObject : meshRenderer->GetRenderObjects()) {
				if (renderObject.material->passType == MATERIAL_PASS_TYPE::OPAQUE) {
					drawContext_.OpaqueObjects.push_back(renderObject);
				}
				else {
					drawContext_.TransparentObjects.push_back(renderObject);
				}
			}
		}
	}

	auto mainCam = GetMainCamera();
	GpuSceneData& sceneData = drawContext_.sceneData;
	sceneData.view = mainCam->GetViewMatrix();
	sceneData.proj = mainCam->GetProjectionMatrix();
	sceneData.proj[1][1] *= -1;
	sceneData.viewproj = sceneData.proj * sceneData.view;

}

void Scene::ClearResourses()
{
    auto instance = RendererVulkan::GetInstance();

    descriptorPool.DestroyPools();
    instance->DestroyGpuBuffer(materialDataBuffer);

    for (auto& [k, v] : meshesMap) {

		instance->DestroyGpuBuffer(v->meshBuffers.indexBuffer);
		instance->DestroyGpuBuffer(v->meshBuffers.vertexBuffer);
    }

    for (auto& [k, v] : imagesMap) {
        
        if (v.image == instance->errorCheckerboardImage.image) {
            //dont destroy the default images
            continue;
        }
        instance->DestroyGpuImage(v);
    }

	for (auto& sampler : samplers) {
		vkDestroySampler(instance->GetVkDevice(), sampler, nullptr);
    }
}
