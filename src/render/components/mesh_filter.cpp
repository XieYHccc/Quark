#include "./mesh_filter.h"

#include "../mesh_factory.h"


void MeshFilter::load_mesh(const std::string& path) { 

    MeshFactory factory;
    mesh_ = factory.load_from_obj(path); 
}

void MeshFilter::make_plane() {
    MeshFactory factory;
    mesh_ = factory.create_plane();
}

std::shared_ptr<TriMesh> MeshFilter::trimesh() {

    if (mesh_ == nullptr)
        return nullptr;

    if (trimesh_ == nullptr)
        trimesh_ = std::make_shared<TriMesh>(mesh_);

    return trimesh_;
}