#pragma once

#include "component.h"

#include "../mesh.h"
#include "../mesh_importer.h"

class MeshFilter : public Component {
public:
    MeshFilter();
    ~MeshFilter() ;
public:
    Mesh* mesh() {return mesh_; }
    void load_mesh(std::string path) { mesh_ = importer_.load_from_obj(path); };
    
private:
    Mesh* mesh_;
    MeshImporter importer_;
};