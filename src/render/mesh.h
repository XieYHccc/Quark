#pragma once

#include <vector>
#include <glm/glm.hpp>

enum MESH_TYPE {
    INVALID,
    CODE,
    CUBE,
    SPHERE,
    PLANE,
    DISK,
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Mesh {
    size_t num_vertex;
    size_t num_index;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    MESH_TYPE type_;

    Mesh() : num_vertex(0), num_index(0), type_{MESH_TYPE::INVALID} {}
};