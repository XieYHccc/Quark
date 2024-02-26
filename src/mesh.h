#pragma once

#include <vector>
#include <glm/glm.hpp>

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

    Mesh() : num_vertex(0), num_index(0) {}
};