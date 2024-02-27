#include "./mesh_filter.h"

#include <rttr/registration.h>

using namespace rttr;
RTTR_REGISTRATION
{
    registration::class_<MeshFilter>("MeshFilter")
            .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

MeshFilter::MeshFilter() {
    mesh_ = nullptr;
}

MeshFilter::~MeshFilter() {
    if (mesh_ != nullptr) {
        delete mesh_;
        mesh_ = nullptr;
    }
}

void MeshFilter::load_mesh(std::string path) { 
    if (mesh_ != nullptr) delete mesh_;

    mesh_ = importer_.load_from_obj(path); 
}

void MeshFilter::make_plane() {
    if (mesh_ != nullptr) delete mesh_;

    mesh_ = importer_.create_plane();
}