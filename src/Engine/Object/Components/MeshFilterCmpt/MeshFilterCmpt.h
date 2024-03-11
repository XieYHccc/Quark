#pragma once

#include <string>

#include "../Component.h"
#include "../../../Render/mesh.h"
#include "../../../Geometry/triangle_mesh.h"

class Object;
class MeshFilterCmpt : public Component {
public:
    MeshFilterCmpt(Object* object) : Component(object) {}

public:

    std::shared_ptr<Mesh> mesh() {return mesh_; }

    std::shared_ptr<TriMesh> trimesh();

    void load_mesh(const std::string& path);

    void make_plane();

private:
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<TriMesh> trimesh_;
};