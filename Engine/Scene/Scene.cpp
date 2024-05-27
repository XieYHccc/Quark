#include "Scene/Scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "GameObject/Components/MeshCmpt.h"
#include "Renderer/Renderer.h"

Scene::Scene(const std::string& name)
{
	this->name = name;
	
	root_ = std::make_unique<GameObject>("root");
}

Scene::~Scene()
{
    auto& instance = Renderer::Instance();

	for (auto& obj : gameObjects_) {
		obj.reset();
	}
	root_.reset();
	
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

	CORE_LOGD("Add Object \"{}\"", name);
	
	return gameObjects_.back().get();
}



