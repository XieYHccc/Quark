#pragma once

#include "component.h"

#include "../mesh.h"
#include "../mesh_importer.h"

class MeshFilter : public Component {
public:
    MeshFilter();

    Mesh* mesh() {return mesh_; }
    
private:
    Mesh* mesh_;
    MeshImporter importer_;
};