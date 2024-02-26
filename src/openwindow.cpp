#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "./Visualization/Viewer.h"
#include "mesh.h"
#include "mesh_importer.h"
#include "material_importer.h"


int main()
{
    Viewer viewer("test", 1000, 700);

    Shader shader("/Users/xieyhccc/develop/learnopengl/src/Visualization/shader.vs", "/Users/xieyhccc/develop/learnopengl/src/Visualization/shader.fs");
    viewer.renderer.set_shader(&shader);

    MaterialImporter mtl_importer;
    auto mtl = mtl_importer.load_from_mtl("/Users/xieyhccc/develop/learnopengl/src/resources/objects/cyborg/cyborg.obj");
    viewer.renderer.set_material(mtl);

    MeshImporter mesh_importer;
    auto mesh = mesh_importer.load_from_obj("/Users/xieyhccc/develop/learnopengl/src/resources/objects/cyborg/cyborg.obj");
    viewer.renderer.setup(mesh);

    viewer.run();
    delete mesh;


}


