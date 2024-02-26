#include "mesh_renderer.h"
#include "./Visualization/Shader.h"
#include "material.h"
#include "mesh.h"
#include "texture.h"

MeshRenderer::MeshRenderer() {
    shader_ = nullptr;
    material_ = nullptr;
    num_vertex = 0;
    num_index = 0;
    vertex_array_object_ = 0;
    vertex_buffer_object_ = 0;
    element_buffer_object_ = 0;

}
bool MeshRenderer::valid() const {
    if (vertex_array_object_ != 0)
        return true;
    else
        return false;
}

MeshRenderer::~MeshRenderer() {
    // if (shader_ != nullptr) {
    //     delete shader_;
    //     shader_ = nullptr;
    // }
    if (material_ != nullptr) {
        delete material_;
        material_ = nullptr;
    }
}

void MeshRenderer::render() {

    if (vertex_array_object_ == 0) {
        std::cerr << "The Vao has not been setup." << std::endl;
    }

    shader_->use();
    // set mvp matrix
    glm::mat4 mvp = projection_ * view_ * model_;
    shader_->setMat4("mvp", mvp);
    // set textures
    std::vector<std::pair<std::string,Texture*>>& textures = material_->textures;
    for (size_t i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,textures[i].second->get_id());

        shader_->setInt(textures[i].first.c_str(), i);
    }
    // draw 
    glBindVertexArray(vertex_array_object_);
    glDrawElements(GL_TRIANGLES, num_index, GL_UNSIGNED_INT, 0);

}

void MeshRenderer::setup(Mesh* mesh) {
    
    num_vertex = mesh->num_vertex;
    num_index = mesh->num_index;

    GLuint shader_id = shader_->get_id(); 
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