#include "./asset_manager.h"

void AssetManager::add_mesh(const std::string& name, std::shared_ptr<Mesh> mesh) {
	mesh_map.emplace(name, mesh);
}

std::shared_ptr<Mesh> AssetManager::load_mesh(const std::string& path) {
	auto target = mesh_map.find(path);
	if (target != mesh_map.end())
		return target->second;

	MeshFactory factory;
	auto mesh = factory.load_from_obj(path);
	mesh_map.emplace(path, mesh);

	return mesh;
}

std::shared_ptr<Texture2D> AssetManager::load_texture(const std::string& path) {
	auto target = texture_map.find(path);
	if (target != texture_map.end())
		return target->second;

	auto texture =  Texture2D::load_texture(path);
	texture_map.emplace(path, texture);

	return texture;
}

std::shared_ptr<Mesh> AssetManager::get_mesh(const std::string& path) {
	auto target = mesh_map.find(path);
	if (target != mesh_map.end())
		return target->second;
}

std::shared_ptr<Texture2D> AssetManager::get_texture(const std::string& path) {
	auto target = texture_map.find(path);
	if (target != texture_map.end())
		return target->second;
}