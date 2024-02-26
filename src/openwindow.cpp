#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "./Visualization/Viewer.h"
#include "./components/mesh_filter.h"
#include "./components/transform.h"
#include "./components/mesh_displayer.h"
#include "./components/object.h"
#include "material_importer.h"


int main()
{
    Viewer viewer("test", 1000, 700);

    Object* obj = new Object("test");
    auto transform = dynamic_cast<Transform*>(obj->add_component("Transform"));
    auto mesh_filter = dynamic_cast<MeshFilter*>(obj->add_component("MeshFilter"));
    auto mesh_displayer = dynamic_cast<MeshDisplayer*>(obj->add_component("MeshDisplayer"));
    // set shader
    Shader* shader = new Shader("/Users/xieyhccc/develop/learnopengl/src/Visualization/shader.vs", "/Users/xieyhccc/develop/learnopengl/src/Visualization/shader.fs");
    mesh_displayer->set_shader(shader);
    // set material
    MaterialImporter mtl_importer;
    auto mtl = mtl_importer.load_from_mtl("/Users/xieyhccc/develop/learnopengl/src/resources/objects/cyborg/cyborg.obj");
    mesh_displayer->set_material(mtl);
    // load mesh
    MeshImporter mesh_importer;
    mesh_filter->load_mesh("/Users/xieyhccc/develop/learnopengl/src/resources/objects/cyborg/cyborg.obj");

    viewer.run();
    delete shader;

}


