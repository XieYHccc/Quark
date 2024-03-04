#include "./mesh_renderer.h"

#include "./shader.h"
#include "./material.h"
#include "./mesh.h"
#include "./texture2d.h"

MeshRenderer::MeshRenderer() {
    shader_ = nullptr;
    material_ = nullptr;
    num_vertex = 0;
    num_index = 0;
    vertex_array_object_ = 0;
    vertex_buffer_object_ = 0;
    element_buffer_object_ = 0;

}
bool MeshRenderer::valid_vao() const {
    if (vertex_array_object_ != 0)
        return true;
    else
        return false;
}

bool MeshRenderer::ready_to_render() const {
    if (vertex_array_object_ == 0) {
        std::cerr << "MeshRenderer::ready_to_render()::vao has not been setup." << std::endl;
        return false;
    }
    if (shader_->get_id() == 0) {
        std::cerr << "MeshRenderer::ready_to_render()::shader has not been setup." << std::endl;
        return false;
    }
    if (material_->textures.empty()) {
        std::cerr << "MeshRenderer::ready_to_render()::material has not been setup." << std::endl;
        return false;
    }

    return true;
}

void MeshRenderer::render() {

    if (!ready_to_render()) { return; }

    shader_->use();
    // set mvp matrix
    glm::mat4 mvp = projection_ * view_ * model_;
    shader_->setMat4("mvp", mvp);
    // set textures
    auto& textures = material_->textures;
    for (size_t i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,textures[i].second->get_id());

        shader_->setInt(textures[i].first.c_str(), i);
    }
    // draw 
    glBindVertexArray(vertex_array_object_);
    glDrawElements(GL_TRIANGLES, num_index, GL_UNSIGNED_INT, 0);

}

void MeshRenderer::setup_vao(std::shared_ptr<Mesh> mesh) {

    num_vertex = mesh->num_vertex;
    num_index = mesh->num_index;

    GLuint shader_id = shader_->get_id(); 
    if (shader_id == 0) {
        std::cerr << "MeshRenderer::setup_vao::The shader has not been setup.";
    }
    GLint vpos_location = glGetAttribLocation(shader_id, "aPos");
    GLint vnor_location = glGetAttribLocation(shader_id, "aNormal");
    GLint vuv_location = glGetAttribLocation(shader_id, "aTexCoord");

    glGenVertexArrays(1, &vertex_array_object_);
    glGenBuffers(1, &vertex_buffer_object_);
    glGenBuffers(1, &element_buffer_object_);

    glBindVertexArray(vertex_array_object_);

    // upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object_);
    glBufferData(GL_ARRAY_BUFFER, mesh->num_vertex * sizeof(Vertex), &mesh->vertices[0], GL_STATIC_DRAW);
    // upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_index * sizeof(unsigned int), &mesh->indices[0], GL_STATIC_DRAW);

    // set up attribute pointers
    // position
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)0);
    // normal
    glEnableVertexAttribArray(vnor_location);
    glVertexAttribPointer(vnor_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // uv
    glEnableVertexAttribArray(vuv_location);
    glVertexAttribPointer(vuv_location, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}