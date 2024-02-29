#pragma once

#include <vector>

#include <glm/glm.hpp>

struct Mesh;
class TriMesh {
public:
	struct Triangle {
		size_t indices[3];
	};

public:
	TriMesh() = default;
	TriMesh(Mesh* mesh);

public:
	void set_positions(const std::vector<glm::vec3>& positions) { positions_ = positions; }
	void set_triangles(const std::vector<Triangle> triangles) { triangles_ = triangles; }

	std::vector<glm::vec3>& get_positions() { return positions_; }
	const std::vector<glm::vec3>& get_positions() const { return positions_; }
	std::vector<Triangle>& get_triangles() { return triangles_; }
	const std::vector<Triangle>& get_triangles() const { return triangles_; }

private:
	std::vector<glm::vec3> positions_;
	std::vector<Triangle> triangles_;
};