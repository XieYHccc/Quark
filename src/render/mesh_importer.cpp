#include "./mesh_importer.h"

#include <iostream>
#include <array>

#include "./mesh.h"
Mesh* MeshImporter::load_from_obj(const std::string& path) {

    tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig config;
	config.vertex_color = false;
	bool success = reader.ParseFromFile(path, config);

	if (!reader.Warning().empty())
		std::cout << reader.Warning() << std::endl;

	if (!reader.Error().empty())
		std::cerr << reader.Error() << std::endl;

	if (!success)
		return nullptr;

    const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();
	const auto& materials = reader.GetMaterials();

    // helpers
    std::map<std::array<int, 3>, unsigned int> vertexIndexMap;
    std::vector<Vertex> vertex_array;
    std::vector<unsigned int> index_array;
    
    // Loop over shapes
	for (size_t s = 0; s < shapes.size(); ++s) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			auto fv = shapes[s].mesh.num_face_vertices[f];
			if (fv != 3) return nullptr; 
			// Loop over vertices in the face.
            std::array<unsigned int, 3> face;
			for (size_t v = 0; v < fv; v++) {
                Vertex vert;
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				std::array<int, 3> key_idx = { idx.vertex_index,
                                               idx.normal_index,
                                               idx.texcoord_index };

				// auto target = vertexIndexMap.find(key_idx);
				// if (target != vertexIndexMap.end()) {
				// 	face[v] = target->second;
				// 	continue;
				// }

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				vert.position = glm::vec3(vx, vy, vz);
				if (idx.normal_index != -1) {
					tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
					tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
					tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
					vert.normal = glm::vec3(nx, ny, nz);
				}
				if (idx.texcoord_index != -1) {
					tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					vert.uv = glm::vec2(tx, 1.0 - ty);
				}

				face[v] = static_cast<unsigned int>(vertex_array.size());
				vertexIndexMap[key_idx] = vertex_array.size();
				vertex_array.push_back(vert);
			}
			index_offset += fv;
			index_array.push_back(face[0]);
			index_array.push_back(face[1]);
			index_array.push_back(face[2]);
        }
	}

    Mesh* mesh = new Mesh();
    mesh->num_vertex = vertex_array.size();
    mesh->num_index = index_array.size();
    mesh->vertices = vertex_array;
    mesh->indices = index_array;
	mesh->type_ = MESH_TYPE::CODE;

    return mesh;
}

Mesh* MeshImporter::create_plane() {
	float vertex_data[32] = {
		// positions          // normals           // texture coords
		 0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // top right
		 0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		-0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f  // top left 
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	Mesh* mesh = new Mesh();
	mesh->type_ = MESH_TYPE::PLANE;
	mesh->num_vertex = 4;
	mesh->num_index = 6;

	for (size_t i = 0; i < 4; ++i) {
		Vertex v;
		v.position = glm::vec3(vertex_data[8 * i], vertex_data[8 * i + 1], vertex_data[8 * i + 2]);
		v.normal = glm::vec3(vertex_data[8 * i + 3], vertex_data[8 * i + 4], vertex_data[8 * i + 5]);
		v.uv = glm::vec2(vertex_data[8 * i + 6], vertex_data[8 * i + 7]);
		mesh->vertices.push_back(v);
	}
	for (size_t i = 0; i < 6; ++i)
		mesh->indices.push_back(indices[i]);

	return mesh;
}
