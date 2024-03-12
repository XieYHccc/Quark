#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "../Object/Object.h"

class SceneMngr {
public:
	std::unordered_map<std::string, std::shared_ptr<Object>> object_map;

public:
	static SceneMngr& Instance() {
		static SceneMngr instance;
		return instance;
	}
	void Initialize() {};
	void Finalize() {};

	void add_object(std::shared_ptr<Object> object);

	std::shared_ptr<Object> get_object(const std::string& name);

private:
	SceneMngr() {}

};