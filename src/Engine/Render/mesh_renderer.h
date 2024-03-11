#pragma  once

#include <memory>

#include <glm/glm.hpp>

struct Material;
struct Mesh;
class Shader;

class MeshRenderer{
public:
    MeshRenderer();
    ~MeshRenderer() = default;

public:
    // shader, material, and VAO are required to render a mesh
    void render();
    bool valid_vao() const;
    
    std::shared_ptr<Material> get_material() { return material_; }
    std::shared_ptr<Shader> get_shader() { return shader_; }

    void set_material(std::shared_ptr<Material> material) { material_ = material; }
    void set_shader(std::shared_ptr<Shader> shader) { shader_ = shader; }
    void set_model_matrix(const glm::mat4& model) { model_ = model;};
    void set_view_matrix(const glm::mat4& view) { view_ = view;};
    void set_projection_matrix(const glm::mat4& projection) { projection_ = projection; };
    // call this fuonction after setting shader
    void setup_vao(std::shared_ptr<Mesh> mesh);

private:
    // determine if shader, material and VAO are valid.
    bool ready_to_render() const;

private:
    std::shared_ptr<Shader> shader_;
    std::shared_ptr<Material> material_;

    glm::mat4 model_;
    glm::mat4 view_;
    glm::mat4 projection_;

    size_t num_vertex;
    size_t num_index;

    unsigned int vertex_buffer_object_;
    unsigned int element_buffer_object_;
    unsigned int vertex_array_object_;
};