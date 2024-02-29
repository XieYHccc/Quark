#pragma once

#include "../mesh_renderer.h"
#include "../material_importer.h"
#include "../../basic/component.h"
#include "../../geometry/bounding_box.h"
class MeshDisplayer : public Component {
public:
    MeshDisplayer();

public: 
    void render();

    void set_material(Material* material) { renderer_.set_material(material); }
    void set_shader(Shader* shader) { renderer_.set_shader(shader); }
private:
    MeshRenderer renderer_;
    MaterialImporter mtl_importer;
    BoundingBox aabb_;
};