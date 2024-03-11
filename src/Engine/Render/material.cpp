#include "./material.h"

#include <iostream>
#include <filesystem>

#include <tiny_obj_loader.h>

#include "./texture2d.h"

std::shared_ptr<Material> Material::load_from_mtl(const std::string& obj_path) {
    std::filesystem::path path(obj_path);
    std::string dir = path.parent_path().string();
    tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig config;
	config.vertex_color = false;
	bool success = reader.ParseFromFile(obj_path, config);

	if (!reader.Warning().empty())
		std::cout << reader.Warning() << std::endl;

	if (!reader.Error().empty())
		std::cerr << reader.Error() << std::endl;
    
    if (!success) {
        return nullptr;
    }

    std::shared_ptr<Material> material = std::make_shared<Material>();
	const auto& materials = reader.GetMaterials();
    for (auto mat_itr = materials.begin(); mat_itr != materials.end(); ++mat_itr) {
        if (!mat_itr->diffuse_texname.empty()) {
            material->textures.push_back(std::make_pair("material.tex_diffuse", Texture2D::load_texture(dir +"/"+ mat_itr->diffuse_texname)));
        }
    }
    
    return material;
}