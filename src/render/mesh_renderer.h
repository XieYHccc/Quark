#pragma  once

#include <glm/glm.hpp>

struct Material;
struct Mesh;
class Shader;

class MeshRenderer{
public:
    MeshRenderer();
    ~MeshRenderer();

public:
    // shader, material, and VAO are required to render a mesh
    void render();
    bool valid_vao() const;
    
    Material* get_material() { return material_; }
    Shader* get_shader() { return shader_; }

    void set_material(Material* material) { material_ = material; }
    void set_shader(Shader* shader) { shader_ = shader; }
    void set_model_matrix(const glm::mat4& model) { model_ = model;};
    void set_view_matrix(const glm::mat4& view) { view_ = view;};
    void set_projection_matrix(const glm::mat4& projection) { projection_ = projection; };
    // call this fuonction after setting shader
    void setup_vao(Mesh* mesh);

private:
    // determine if shader, material and VAO are valid.
    bool ready_to_render() const;

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