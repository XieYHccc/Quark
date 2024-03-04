#pragma once
#include <string>
#include <memory>

#include <tiny_obj_loader.h>

struct Mesh;
struct Vertex;
class MeshFactory {
public:
    MeshFactory() {};

public:
    std::shared_ptr<Mesh> load_from_obj(const std::string& path);

    std::shared_ptr<Mesh> create_mesh(unsigned int num_vertex, unsigned int num_indexconst,
                      Vertex* vertex_data, const unsigned int* index_data);

    std::shared_ptr<Mesh> create_plane();
};