#pragma once
#include <string>

#include <tiny_obj_loader.h>

struct Mesh;
struct Vertex;
class MeshImporter {
public:
    MeshImporter() {};

public:
    Mesh* load_from_obj(const std::string& path);

    Mesh* create_mesh(unsigned int num_vertex, unsigned int num_indexconst,
                      Vertex* vertex_data, const unsigned int* index_data);

    Mesh* create_plane();
};