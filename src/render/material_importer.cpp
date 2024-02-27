#include "./material_importer.h"

#include <iostream>
#include <filesystem>

#include <tiny_obj_loader.h>

#include "./material.h"
#include "./texture.h"

Material* MaterialImporter::load_from_mtl(std::string obj_path) {
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

    Material* material = new Material();
	const auto& materials = reader.GetMaterials();
    for (auto mat_itr = materials.begin(); mat_itr != materials.end(); ++mat_itr) {
        if (!mat_itr->diffuse_texname.empty()) {
            material->textures.push_back(std::make_pair("material.tex_diffuse", Texture::load_texture(dir +"/"+ mat_itr->diffuse_texname)));
        }
    }
    
    return material;
}