#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "./viewer.h"
#include "../core/transform.h"
#include "../core/object.h"
#include "../render/components/mesh_displayer.h"
#include "../render/components/mesh_filter.h"
#include "../render/material_importer.h"
#include "../render/texture.h"
#include "../render/material.h"


int main()
{
    Viewer viewer("test", 2000, 1200);

    Object* obj = new Object("obj");
    auto transform = dynamic_cast<Transform*>(obj->add_component("Transform"));
    auto mesh_filter = dynamic_cast<MeshFilter*>(obj->add_component("MeshFilter"));
    auto mesh_displayer = dynamic_cast<MeshDisplayer*>(obj->add_component("MeshDisplayer"));
    // set shader
    Shader* shader = new Shader("../../src/render/shader.vs", "../../src/render/shader.fs");
    mesh_displayer->set_shader(shader);
    // set material
    MaterialImporter mtl_importer;
    auto mtl = mtl_importer.load_from_mtl("../../resources/objects/cyborg/cyborg.obj");
    mesh_displayer->set_material(mtl);
    // load mesh
    MeshImporter mesh_importer;
    mesh_filter->load_mesh("../../resources/objects/cyborg/cyborg.obj");

    // ===================================================================================
    Object* plane = new Object("plane");
    auto p_transform = dynamic_cast<Transform*>(plane->add_component("Transform"));
    auto p_mesh_filter = dynamic_cast<MeshFilter*>(plane->add_component("MeshFilter"));
    auto p_mesh_displayer = dynamic_cast<MeshDisplayer*>(plane->add_component("MeshDisplayer"));
    p_transform->set_scale(glm::vec3(100.f, 100.f, 100.f));
    p_transform->set_rotation(glm::vec3(90.f, 0.f, 0.f));
    // set shader
    p_mesh_displayer->set_shader(shader);
    // set material
    Texture* tex = Texture::load_texture("../../resources/textures/grid_box_grey.png");
    Material* plane_mtl = new Material();
    plane_mtl->textures.push_back(std::make_pair("material.tex_diffuse", tex));
    p_mesh_displayer->set_material(plane_mtl);
    // load mesh
    p_mesh_filter->make_plane();

    viewer.run();

}


