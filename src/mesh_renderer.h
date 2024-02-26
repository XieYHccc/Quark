#pragma  once

#include <glm/glm.hpp>

class Material;
class Mesh;
class Shader;

class MeshRenderer{
public:
    MeshRenderer();
    ~MeshRenderer();

public:
    void render();
    void setup(Mesh* mesh);
    bool valid() const;
    
    Material* get_material() { return material_; }
    Shader* get_shader() { return shader_; }

    void set_material(Material* material) { material_ = material; }
    void set_shader(Shader* shader) { shader_ = shader; }
    void set_model_matrix(glm::mat4 model) { model_ = model;};
    void set_view_matrix(glm::mat4 view) { view_ = view;};
    void set_projection_matrix(glm::mat4 projection){ projection_ = projection; };

private:
    Shader* shader_;
    Material* material_;

    glm::mat4 model_;
    glm::mat4 view_;
    glm::mat4 projection_;

    size_t num_vertex;
    size_t num_index;

    unsigned int vertex_buffer_object_;
    unsigned int element_buffer_object_;
    unsigned int vertex_array_object_;
};