#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "./viewer.h"
#include "../basic/transform.h"
#include "../basic/object.h"
#include "../render/components/mesh_displayer.h"
#include "../render/components/mesh_filter.h"
#include "../render/texture2d.h"
#include "../render/material.h"
#include "../physics/rigid_body_dynamics.h"
#include "../physics/box_collider.h"
#include "../physics/mesh_collider.h"

int main()
{
    Viewer viewer("test", 2000, 1200);
    auto shader = std::make_shared<Shader>("../../src/render/shader.vs", "../../src/render/shader.fs");

    // add grid box
    // ==========================================================================
    Object* gridbox = new Object("grid box");
    auto box_transform = dynamic_cast<Transform*>(gridbox->add_component("Transform"));
    auto box_mesh_filter = dynamic_cast<MeshFilter*>(gridbox->add_component("MeshFilter"));
    auto box_mesh_displayer = dynamic_cast<MeshDisplayer*>(gridbox->add_component("MeshDisplayer"));
    // load mesh
    box_mesh_filter->make_plane(); 
    // set transform
    box_transform->set_scale(glm::vec3(50.f, 50.f, 50.f));
    box_transform->set_rotation_by_angle(glm::vec3(glm::radians(90.f), 0.f, 0.f));
    // set shader
    box_mesh_displayer->set_shader(shader);
    // set material
    auto gridbox_tex = Texture2D::load_texture("../../resources/textures/marble.jpg");
    auto gridbox_mtl = std::make_shared<Material>();
    gridbox_mtl->textures.push_back(std::make_pair("material.tex_diffuse", gridbox_tex));
    box_mesh_displayer->set_material(gridbox_mtl);

    // add wall
    // ==========================================================================
    Object* wall = new Object("wall");
    auto wall_transform = dynamic_cast<Transform*>(wall->add_component("Transform"));
    auto wall_mesh_filter = dynamic_cast<MeshFilter*>(wall->add_component("MeshFilter"));
    auto wall_mesh_displayer = dynamic_cast<MeshDisplayer*>(wall->add_component("MeshDisplayer"));
    auto wall_box_collider = dynamic_cast<PlaneCollider*>(wall->add_component("PlaneCollider"));
    wall_transform->set_scale(glm::vec3(10.f, 7.f, 1.f));
    wall_transform->set_position(glm::vec3(0.f, 0.f, -3.f));
    // set shader
    wall_mesh_displayer->set_shader(shader);
    // set material
    auto wall_tex = Texture2D::load_texture("../../resources/textures/bricks2.jpg");
    auto wall_mtl = std::make_shared<Material>();
    wall_mtl->textures.push_back(std::make_pair("material.tex_diffuse", wall_tex));
    wall_mesh_displayer->set_material(wall_mtl);
    // load mesh
    wall_mesh_filter->make_plane();
    wall_box_collider->awake();

    // add bunny
    // ===========================================================================
    Object* obj = new Object("bunny");
    auto transform = dynamic_cast<Transform*>(obj->add_component("Transform"));
    auto mesh_filter = dynamic_cast<MeshFilter*>(obj->add_component("MeshFilter"));
    auto mesh_displayer = dynamic_cast<MeshDisplayer*>(obj->add_component("MeshDisplayer"));
    auto rigid_body = dynamic_cast<RigidBodyDynamic*>(obj->add_component("RigidBodyDynamic"));
    auto mesh_collider = dynamic_cast<MeshCollider*>(obj->add_component("MeshCollider"));
    transform->set_position(glm::vec3(0.f, 1.f, 0.f));
    transform->set_scale(glm::vec3(2.f, 2.f, 2.f));
    // set shader
    mesh_displayer->set_shader(shader);
    // set material
    auto tex = Texture2D::load_texture("../../resources/textures/bunny.jpg");
    auto mtl = std::make_shared<Material>();
    mtl->textures.push_back(std::make_pair("material.tex_diffuse", tex));
    mesh_displayer->set_material(mtl);
    // load mesh
    mesh_filter->load_mesh("../../resources/objects/Stanford_Bunny.obj");
    rigid_body->awake();
    mesh_collider->awake();

    viewer.run();

    delete gridbox;
    delete wall;
    delete obj;
}


