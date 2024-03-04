#include "./scene.h"

#include <iostream>
void Scene::add_object(std::shared_ptr<Object> object) {
	auto target = object_map.find(object->get_name());
	if (target != object_map.end()) {
		std::cerr << "Scene::add_object: object has existed." << std::endl;
		return;
	}

	object_map.emplace(object->get_name(), object);
}

std::shared_ptr<Object> Scene::get_object(const std::string& name) {
	auto target = object_map.find(name);
	if (target == object_map.end()) {
		std::cerr << "Scene::add_object: object has existed." << std::endl;
		return nullptr;
	}
	else
		return target->second;
}