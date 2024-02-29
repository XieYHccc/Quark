#include "./triangle_mesh.h"

#include <iostream>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "../render/mesh.h"


TriMesh::TriMesh(Mesh* mesh) {
	if (mesh == nullptr)
		return;

	// The number of mesh's index must be multiples of  3
	if (mesh->num_index % 3 != 0) {
		std::cerr << "TriMesh::TriMesh(Mesh*): only support triangle mesh" << std::endl;
		return;
	}

	std::unordered_map<glm::vec3, size_t> unique_pos;
	std::vector<glm::vec3> positions;
	std::vector<Triangle> triangles;

	int num_triangles = mesh->num_index / 3;
	auto vertices = mesh->vertices;
	auto indices = mesh->indices;
	// loop mesh
	for (size_t i = 0; i < num_triangles; ++i) {
		Triangle tri;
		// loop face
		for (size_t j = 0; j < 3; ++j) {
			size_t idx = indices[3 * i + j];
			glm::vec3 pos = vertices[idx].position;
			if (unique_pos.find(pos) != unique_pos.end()) {
				tri.indices[j] = unique_pos[pos];
			}
			else {
				tri.indices[j] = positions.size();
				unique_pos[pos] = positions.size();
				positions.push_back(pos);
			}
		}
		triangles.push_back(tri);
	}

	triangles_ = triangles;
	positions_ = positions;
}