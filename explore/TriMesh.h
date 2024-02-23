#pragma once
#include <vector>

#include <glm/glm.hpp>
#include <XMath/Algebra/Point.h>

#include "./Visualization/Shader.h"
#include "./Visualization/Texture.h"

class TriMesh {
public :
	using vec3f = xyh::XMath::vec3;
	using point3f = xyh::XMath::point3;
	using point2f = xyh::XMath::point2;


	struct Triangle {
		uint32_t indices[3];

		Triangle(uint32_t i0, uint32_t i1, uint32_t i2) {
			indices[0] = i0;
			indices[1] = i1;
			indices[2] = i2;
		}
	};

public:
	TriMesh();
	TriMesh(const char* filename) : TriMesh() {}
	TriMesh(const std::vector<unsigned int>& indices,
		const std::vector<point3f>& positions,
		const std::vector<vec3f>& normals = std::vector<vec3f>(),
		const std::vector<point2f>& texcoords = std::vector<point2f>())
	:TriMesh() {
		init(indices, positions, normals, texcoords);
	}
	void init(const std::vector<unsigned int>& indices,
		const std::vector<point3f>& positions,
		const std::vector<vec3f>& normals = std::vector<vec3f>(),
		const std::vector<point2f>& texcoords = std::vector<point2f>());
	

public:
	const std::vector<point3f>& positions() const { return positions_; }
	std::vector<point3f>& positions() { return positions_; }
	const std::vector<Triangle>& triangles() const { return triangles_; }
	std::vector<Triangle>& triangles() { return triangles_; }
	size_t nverts() { return positions_.size(); }
	size_t nfaces() { return triangles_.size(); }
	point3f position(size_t i) { return positions_[i]; }

	void draw(const Shader& shader);

public:
	void SetPositions(std::vector<point3f> positions);
	void SetUV(std::vector<point2f> uv);
	void SetNormals(std::vector<vec3f> normals);
	void SetTriangles(std::vector<Triangle> triangles);
	void SetNormalIdx(std::vector<size_t> indices);
	void SetUVIdx(std::vector<size_t> indices);
	bool LoadTexture(const char* file);

public: 
	static TriMesh generate_plane();

private:
	void SetupVAO();

private:
	// Mesh geometry attributes
	std::vector<Triangle> triangles_;
	std::vector<point3f> positions_;
	std::vector<vec3f> normals_;
	std::vector<point2f> texcoords_;

	// Indices
	std::vector<size_t> nor_idx_;
	std::vector<size_t> tex_idx_;

	// OpenGL data
	unsigned int VAO_, EBO_;
	unsigned int position_buffer_;
	unsigned int texcoords_buffer_;
	unsigned int normal_buffer_;
	
	Texture diffuse_;
};