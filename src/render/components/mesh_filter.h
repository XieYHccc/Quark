#pragma once

#include "../../core/component.h"

#include "../mesh.h"
#include "../mesh_importer.h"

class MeshFilter : public Component {
public:
    MeshFilter();
    ~MeshFilter();

public:
    Mesh* mesh() {return mesh_; }
    void load_mesh(std::string path);
    void make_plane();

private:
    Mesh* mesh_;
    MeshImporter importer_;
};