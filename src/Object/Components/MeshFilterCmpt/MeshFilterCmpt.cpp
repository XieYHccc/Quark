#include "./MeshFilterCmpt.h"

#include "../../../render/mesh_factory.h"


void MeshFilterCmpt::load_mesh(const std::string& path) {

    MeshFactory factory;
    mesh_ = factory.load_from_obj(path); 
}

void MeshFilterCmpt::make_plane() {
    MeshFactory factory;
    mesh_ = factory.create_plane();
}

std::shared_ptr<TriMesh> MeshFilterCmpt::trimesh() {

    if (mesh_ == nullptr)
        return nullptr;

    if (trimesh_ == nullptr)
        trimesh_ = std::make_shared<TriMesh>(mesh_);

    return trimesh_;
}