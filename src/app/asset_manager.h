#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../render/mesh.h"
#include "../render/mesh_factory.h"
#include "../render/texture2d.h"

class AssetManager {
public:
	static AssetManager& Instance() {
		static AssetManager instance;
		return instance;
	}
	void add_mesh(const std::string& name, std::shared_ptr<Mesh>);

	std::shared_ptr<Mesh> load_mesh(const std::string& path);
	std::shared_ptr<Texture2D> load_texture(const std::string& path);

	std::shared_ptr<Mesh> get_mesh(const std::string& path);
	std::shared_ptr<Texture2D> get_texture(const std::string& path);

private:
	AssetManager();

private:
	std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_map;
	std::unordered_map<std::string, std::shared_ptr<Texture2D>> texture_map;
};
