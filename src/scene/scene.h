#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "../basic/object.h"

class Scene {
public:
	std::unordered_map<std::string, std::shared_ptr<Object>> object_map;

public:
	static Scene& Instance() {
		static Scene instance;
		return instance;
	}
	void add_object(std::shared_ptr<Object> object);

	std::shared_ptr<Object> get_object(const std::string& name);

private:
	Scene() {}

};