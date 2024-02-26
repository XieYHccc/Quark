#pragma once

#include "component.h"

#include "../mesh_renderer.h"

class MeshDisplayer : Component {
public:
    MeshDisplayer();

public: 
    void render();
    void set_material(Material* material) { renderer_.set_material(material); }
    void set_shader(Shader* shader) { renderer_.set_shader(shader); }
    // void set_model_matrix(glm::mat4 model) { renderer_.set_model_matrix(model); };
    // void set_view_matrix(glm::mat4 view) { renderer_.set_view_matrix(view); };
    // void set_projection_matrix(glm::mat4 projection) { renderer_.set_projection_matrix(projection); };
private:
    MeshRenderer renderer_;
};