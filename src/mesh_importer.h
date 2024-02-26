#pragma once
#include <string>

#include <tiny_obj_loader.h>

class Mesh;
class Vertex;
class MeshImporter {
public:
    MeshImporter() {};

public:
    Mesh* load_from_obj(const std::string& path);

    Mesh* create_mesh(unsigned int num_vertex, unsigned int num_indexconst,
                      Vertex* vertex_data, const unsigned int* index_data);


};