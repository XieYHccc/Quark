#pragma once

#include <string>

#include "../../basic/component.h"
#include "../mesh.h"
#include "../../geometry/triangle_mesh.h"

class MeshFilter : public Component {
public:
    MeshFilter();
    ~MeshFilter();

public:

    std::shared_ptr<Mesh> mesh() {return mesh_; }

    std::shared_ptr<TriMesh> trimesh();

    void load_mesh(const std::string& path);

    void make_plane();

private:
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<TriMesh> trimesh_;
};