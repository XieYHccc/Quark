#include "TriMesh.h"

#include <fstream>
#include <sstream>
#include <string>
#include <glad/glad.h>

TriMesh::TriMesh() {
    VAO_ = 0;
    EBO_ = 0;
    position_buffer_ = 0;
    normal_buffer_ = 0;
    texcoords_buffer_ = 0;
}

void TriMesh::SetPositions(std::vector<point3f> positions) {
    positions_ = positions;
}

void TriMesh::SetUV(std::vector<point2f> uv) {
    texcoords_ = uv;
}

void TriMesh::SetNormals(std::vector<vec3f> normals) {
    normals_ = normals;
}

void TriMesh::SetTriangles(std::vector<Triangle> triangles) {
    triangles_ = triangles;
}

void TriMesh::SetNormalIdx(std::vector<size_t> indices) {
    nor_idx_ = indices;
}

void TriMesh::SetUVIdx(std::vector<size_t> indices) {
    tex_idx_ = indices;
}

void TriMesh::init(const std::vector<unsigned int>& indices,
    const std::vector<point3f>& positions,
    const std::vector<vec3f>& normals,
    const std::vector<point2f>& texcoords) 
{
    if (!(indices.size() > 0 && indices.size() % 3 == 0)
        || positions.size() <= 0
        || !normals.empty() && normals.size() != positions.size()
        || !texcoords.empty() && texcoords.size() != positions.size())
    {
        printf("ERROR: TriMesh is invalid.\n");
        return;
    }

    positions_ = positions;

    for (size_t i = 0; i < indices.size(); i += 3)
        triangles_.emplace_back(indices[i], indices[i + 1], indices[i + 2]);
    if (!texcoords.empty())
        texcoords_ = texcoords;
    if (!normals.empty())
        normals_ = normals;
}

//void TriMesh::SetupVAO() {
//    if (!VAO_) {
//        glGenVertexArrays(1, &VAO_);
//        glBindVertexArray(VAO_);
//        glGenBuffers(1, &EBO_);
//        glGenBuffers(1, &position_buffer_);
//        glGenBuffers(1, &texcoords_buffer_);
//        glGenBuffers(1, &normal_buffer_);
//    }
//
//    // Upload indices
//    if (!triangles_.empty()) {
//        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
//        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles_.size() * sizeof(Triangle), triangles_.data(), GL_STATIC_DRAW);
//    }
//    // upload positions
//    glBindBuffer(GL_ARRAY_BUFFER, position_buffer_);
//    glBufferData(GL_ARRAY_BUFFER, positions_.size() * 3 * sizeof(float), positions_.data(), GL_STATIC_DRAW);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
//    glEnableVertexAttribArray(0);
//    // Upload normals
//    if (!normals_.empty()) {
//        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_);
//        glBufferData(GL_ARRAY_BUFFER, normals_.size() * 3 * sizeof(float), normals_.data(), GL_STATIC_DRAW);
//        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
//        glEnableVertexAttribArray(1);
//    }
//    else {
//        glDisableVertexAttribArray(1);
//    }
//    // Upload texture coordinates
//    if (!texcoords_.empty()) {
//        glBindBuffer(GL_ARRAY_BUFFER, texcoords_buffer_);
//        glBufferData(GL_ARRAY_BUFFER, texcoords_.size() * 2 * sizeof(float), texcoords_.data(), GL_STATIC_DRAW);
//        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
//        glEnableVertexAttribArray(2);
//    }
//    else {
//        glDisableVertexAttribArray(2);
//    }
//}

void TriMesh::SetupVAO() {
    if (!VAO_) {
        glGenVertexArrays(1, &VAO_);
        glBindVertexArray(VAO_);
        glGenBuffers(1, &EBO_);
        glGenBuffers(1, &position_buffer_);
        glGenBuffers(1, &texcoords_buffer_);
        glGenBuffers(1, &normal_buffer_);
    }

    // Upload indices
    if (!triangles_.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles_.size() * sizeof(Triangle), triangles_.data(), GL_STATIC_DRAW);
    }
    // upload positions
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer_);
    glBufferData(GL_ARRAY_BUFFER, positions_.size() * 3 * sizeof(float), positions_.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    // Upload normals
    if (!normals_.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_);
        glBufferData(GL_ARRAY_BUFFER, normals_.size() * 3 * sizeof(float), normals_.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
    }
    else {
        glDisableVertexAttribArray(1);
    }
    // Upload texture coordinates
    if (!texcoords_.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, texcoords_buffer_);
        glBufferData(GL_ARRAY_BUFFER, texcoords_.size() * 2 * sizeof(float), texcoords_.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
    }
    else {
        glDisableVertexAttribArray(2);
    }
}

void TriMesh::draw(const Shader& shader) {
    if (!VAO_)
        SetupVAO();

    shader.use();
    // Bind texture
    if (diffuse_.get_id() != 0)
        glBindTexture(GL_TEXTURE_2D, diffuse_.get_id());
    else
        std::cout << "texture is invalid" << std::endl;
    // draw Mesh
    glBindVertexArray(VAO_);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(3 * triangles_.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

//bool TriMesh::LoadTexture(const char* file) {
//    return diffuse_.load_texture(file);
//}
//
//TriMesh TriMesh::generate_plane()
//{
//    int num_vertex = 4;
//    int num_tri = 2;
//    const float vertexData[32] = {
//        // positions          // normals           // texture coords
//         0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // top right
//         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // bottom right
//        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
//        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 1.0f,   0.0f, 1.0f  // top left 
//    };
//
//    std::vector<point3f> positions(num_vertex);
//    std::vector<vec3f> normals(num_vertex);
//    std::vector<point2f> texcoords(num_vertex);
//    for (unsigned int i = 0; i < num_vertex; ++i) {
//        positions[i] = point3f(vertexData[8 * i], vertexData[8 * i + 1], vertexData[8 * i + 2]);
//        normals[i] = vec3f(vertexData[8 * i + 3], vertexData[8 * i + 4],vertexData[8 * i + 5]);
//        texcoords[i] = point2f(vertexData[8 * i + 6], vertexData[8 * i + 7]);
//    }
//    std::vector<unsigned int> indices = {
//        0, 1, 3, // first triangle
//        1, 2, 3  // second triangle
//    };
//
//    return TriMesh(indices, positions, normals, texcoords);
//    
//}

TriMesh TriMesh::generate_plane()
{
    int num_vertex = 4;
    int num_tri = 2;
    const float vertexData[32] = {
        // positions          // normals           // texture coords
         0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 1.0f,   0.0f, 1.0f  // top left 
    };

    std::vector<point3f> positions;
    std::vector<vec3f> normals;
    std::vector<point2f> texcoords;
    for (unsigned int i = 0; i < num_vertex; ++i) {
        positions[i] = point3f(vertexData[8 * i], vertexData[8 * i + 1], vertexData[8 * i + 2]);
        texcoords[i] = point2f(vertexData[8 * i + 6], vertexData[8 * i + 7]);
    }

    normals.push_back(vec3f(0.f, 0.f, 1.f));
   
    std::vector<size_t> indices = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    std::vector<size_t> nor_idx = {
        0, 0, 0,
        0, 0, 0
    };
    std::vector<Triangle> triangles = {
        Triangle(0, 1, 3),
        Triangle(1, 2, 3)
    };
    TriMesh mesh;
    mesh.SetPositions(positions);
    mesh.SetNormals(normals);
    mesh.SetUV(texcoords);
    mesh.SetNormalIdx(nor_idx);
    mesh.SetTriangles(triangles);
    return mesh;

}