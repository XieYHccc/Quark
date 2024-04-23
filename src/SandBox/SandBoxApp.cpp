#include "SandBoxApp.h"

#include <Application/Window/Input.h>
#include <Scene/SceneMngr.h>
#include <GameObject/Components/TransformCmpt/TransformCmpt.h>
#include <GameObject/Components/MeshRendererCmpt/MeshRenderCmpt.h>
#include <GameObject/Components/MeshFilterCmpt/MeshFilterCmpt.h>

Application* CreateApplication()
{

    auto application = new SandBoxApp("test"," ", 2000, 1200);

    // auto shader = std::make_shared<Shader>("../src/Engine/Render/shader.vs", "../src/Engine/Render/shader.fs");

    // // add grid box
    // // ==========================================================================
    // auto gridbox = std::make_shared<Object>("gridbox");
    // auto gridbox_transform = gridbox->add_component<Transform>();
    // auto gridbox_mesh_filter = gridbox->add_component<MeshFilterCmpt>();
    // auto gridbox_displayer = gridbox->add_component<MeshRendererCmpt>();
    // auto gridbox_plane_collider = gridbox->add_component<PlaneCollider>();

    // gridbox_plane_collider->position = glm::vec3(0.f, 0.f, 0.f);
    // gridbox_plane_collider->normal = glm::vec3(0.f, 1.f, 0.f);
    // // load mesh
    // gridbox_mesh_filter->make_plane(); 
    // // set transform
    // gridbox_transform->set_scale(glm::vec3(50.f, 50.f, 50.f));
    // gridbox_transform->set_rotation_by_angle(glm::vec3(glm::radians(90.f), 0.f, 0.f));
    // // set shader
    // gridbox_displayer->set_shader(shader);
    // // set material
    // auto gridbox_tex = Texture2D::load_texture("../resources/textures/marble.jpg");
    // auto gridbox_mtl = std::make_shared<Material>();
    // gridbox_mtl->textures.push_back(std::make_pair("material.tex_diffuse", gridbox_tex));
    // gridbox_displayer->set_material(gridbox_mtl);

    // // add wall
    // // ==========================================================================
    // auto wall = std::make_shared<Object>("wall");
    // auto wall_transform = wall->add_component<Transform>();
    // auto wall_mesh_filter = wall->add_component<MeshFilterCmpt>();
    // auto wall_mesh_displayer = wall->add_component<MeshRendererCmpt>();
    // auto wall_plane_collider = wall->add_component<PlaneCollider>();
    // wall_plane_collider->position = glm::vec3(0.f, 0.f, -3.f);
    // wall_plane_collider->normal = glm::vec3(0.f, 0.f, 1.f);
    // wall_transform->set_scale(glm::vec3(10.f, 7.f, 1.f));
    // wall_transform->set_position(glm::vec3(0.f, 0.f, -3.f));
    // // set shader
    // wall_mesh_displayer->set_shader(shader);
    // // set material
    // auto wall_tex = Texture2D::load_texture("../resources/textures/bricks2.jpg");
    // auto wall_mtl = std::make_shared<Material>();
    // wall_mtl->textures.push_back(std::make_pair("material.tex_diffuse", wall_tex));
    // wall_mesh_displayer->set_material(wall_mtl);
    // // load mesh
    // wall_mesh_filter->make_plane();

    // // add bunny
    // // ===========================================================================
    // auto bunny = std::make_shared<Object>("bunny");
    // auto transform = bunny->add_component<Transform>();
    // auto mesh_filter = bunny->add_component<MeshFilterCmpt>();
    // auto mesh_displayer = bunny->add_component<MeshRendererCmpt>();
    // auto rigid_body = bunny->add_component<RigidBodyDynamic>();
    // auto mesh_collider = bunny->add_component<MeshCollider>();
    // transform->set_position(glm::vec3(0.f, 1.f, 0.f));
    // transform->set_scale(glm::vec3(2.f, 2.f, 2.f));
    // // set shader
    // mesh_displayer->set_shader(shader);
    // // set material
    // auto tex = Texture2D::load_texture("../resources/textures/bunny.jpg");
    // auto mtl = std::make_shared<Material>();
    // mtl->textures.push_back(std::make_pair("material.tex_diffuse", tex));
    // mesh_displayer->set_material(mtl);
    // // load mesh
    // mesh_filter->load_mesh("../resources/objects/Stanford_Bunny.obj");
    // rigid_body->awake();
    // mesh_collider->awake();

    // // ================================================================
    // SceneMngr::Instance().add_object(gridbox);
    // SceneMngr::Instance().add_object(wall);
    // SceneMngr::Instance().add_object(bunny);

    return application;
}

// void SandBoxApp::Update()
// {
//     if (Input::IsKeyPressed(K)) {
//         auto bunny = SceneMngr::Instance().get_object("bunny");
//         auto transform = bunny->get_component<Transform>();
//         auto rigid_body = bunny->get_component<RigidBodyDynamic>();
//         transform->set_position(glm::vec3(0.f, 1.f, 0.f));
//         transform->set_rotation(glm::quat(1.f, 0.f, 0.f, 0.f));
//         rigid_body->init_velocity();

//     }
//     if (Input::IsKeyPressed(L)) {
//         auto bunny = SceneMngr::Instance().get_object("bunny");
//         auto transform = bunny->get_component<Transform>();
//         auto rigid_body = bunny->get_component<RigidBodyDynamic>();
//         rigid_body->init_velocity(glm::vec3(0.f, 3.f, -6.f));
//         rigid_body->set_lauched(true);
//     }
//     auto bunny = SceneMngr::Instance().get_object("bunny");
//     auto wall = SceneMngr::Instance().get_object("wall");
//     auto gridbox = SceneMngr::Instance().get_object("gridbox");
//     auto mesh_collider = bunny->get_component<MeshCollider>();
//     auto wall_plane_collider = wall->get_component<PlaneCollider>();
//     auto ground_plane_collider = gridbox->get_component<PlaneCollider>();
//     check_collision(wall_plane_collider, mesh_collider);
//     check_collision(ground_plane_collider, mesh_collider);
// }

// void SandBoxApp::Render()
// {
//     glClearColor(0.36f, 0.36f, 0.36f, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//     for (auto& obj : SceneMngr::Instance().object_map) {
//         auto mesh_diplayer = obj.second->get_component<MeshRendererCmpt>();
//         auto rigid_body = obj.second->get_component<RigidBodyDynamic>();
//         if (mesh_diplayer == nullptr)
//             continue;

//         if (rigid_body != nullptr) {
//             rigid_body->update();
//         }

//         mesh_diplayer->render();
//     }
// }