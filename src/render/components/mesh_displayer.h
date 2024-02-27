#pragma once

#include "../../core/component.h"
#include "../mesh_renderer.h"
#include "../material_importer.h"
class MeshDisplayer : public Component {
public:
    MeshDisplayer();

public: 
    void render();
    // void load_material(std::string path) {renderer_.set_material(mtl_importer.load_from_mtl(path)); }
    void set_material(Material* material) { renderer_.set_material(material); }
    void set_shader(Shader* shader) { renderer_.set_shader(shader); }
private:
    MeshRenderer renderer_;
    MaterialImporter mtl_importer;
};