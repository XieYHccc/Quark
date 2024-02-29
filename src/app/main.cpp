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
#include "../physics/rigid_body_dynamics.h"

void release_objects();

int main()
{
    Viewer viewer("test", 2000, 1200);
    Shader* shader = new Shader("../../src/render/shader.vs", "../../src/render/shader.fs");

    // add grid box
    // ==========================================================================
    Object* gridbox = new Object("grid box");
    auto box_transform = dynamic_cast<Transform*>(gridbox->add_component("Transform"));
    auto box_mesh_filter = dynamic_cast<MeshFilter*>(gridbox->add_component("MeshFilter"));
    auto box_mesh_displayer = dynamic_cast<MeshDisplayer*>(gridbox->add_component("MeshDisplayer"));
    box_transform->set_scale(glm::vec3(50.f, 50.f, 50.f));
    box_transform->set_rotation_by_angle(glm::vec3(glm::radians(90.f), 0.f, 0.f));
    // set shader
    box_mesh_displayer->set_shader(shader);
    // set material
    Texture* gridbox_tex = Texture::load_texture("../../resources/textures/marble.jpg");
    Material* gridbox_mtl = new Material();
    gridbox_mtl->textures.push_back(std::make_pair("material.tex_diffuse", gridbox_tex));
    box_mesh_displayer->set_material(gridbox_mtl);
    // load mesh
    box_mesh_filter->make_plane();

    // add wall
    // ==========================================================================
    Object* wall = new Object("wall");
    auto wall_transform = dynamic_cast<Transform*>(wall->add_component("Transform"));
    auto wall_mesh_filter = dynamic_cast<MeshFilter*>(wall->add_component("MeshFilter"));
    auto wall_mesh_displayer = dynamic_cast<MeshDisplayer*>(wall->add_component("MeshDisplayer"));

    wall_transform->set_scale(glm::vec3(10.f, 7.f, 1.f));
    wall_transform->set_position(glm::vec3(0.f, 0.f, -3.f));
    // set shader
    wall_mesh_displayer->set_shader(shader);
    // set material
    Texture* wall_tex = Texture::load_texture("../../resources/textures/bricks2.jpg");
    Material* wall_mtl = new Material();
    wall_mtl->textures.push_back(std::make_pair("material.tex_diffuse", wall_tex));
    wall_mesh_displayer->set_material(wall_mtl);
    // load mesh
    wall_mesh_filter->make_plane();

    // add bunny
    // ===========================================================================
    Object* obj = new Object("bunny");
    auto transform = dynamic_cast<Transform*>(obj->add_component("Transform"));
    auto mesh_filter = dynamic_cast<MeshFilter*>(obj->add_component("MeshFilter"));
    auto mesh_displayer = dynamic_cast<MeshDisplayer*>(obj->add_component("MeshDisplayer"));
    auto rigid_body = dynamic_cast<RigidBodyDynamic*>(obj->add_component("RigidBodyDynamic"));
    transform->set_position(glm::vec3(0.f, 1.f, 0.f));
    transform->set_scale(glm::vec3(2.f, 2.f, 2.f));
    // set shader
    mesh_displayer->set_shader(shader);
    // set material
    Texture* tex = Texture::load_texture("../../resources/textures/bunny.jpg");
    Material* mtl = new Material();
    mtl->textures.push_back(std::make_pair("material.tex_diffuse", tex));
    mesh_displayer->set_material(mtl);
    // load mesh
    MeshImporter mesh_importer;
    mesh_filter->load_mesh("../../resources/objects/Stanford_Bunny.obj");
    rigid_body->awake();

    viewer.run();

    release_objects();
}

void release_objects() {
    for (auto obj : Object::object_list) {
        if (obj != nullptr) {
            delete obj;
            obj = nullptr;
        }
    }
}

