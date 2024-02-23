#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "./Visualization/Shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    // -------------------constructors----------------------
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

    // now that we have all the required data, set the vertex buffers and its attribute pointers.
    setupMesh();
    };

    // -----------------------API---------------------------
     // render the mesh
    void Draw(Shader &shader) 
    {
        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            std::string number;
            std::string name = textures[i].type;
            if(name == "tex_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "tex_specular")
                number = std::to_string(specularNr++); // transfer unsigned int to string
            else if(name == "tex_normal")
                number = std::to_string(normalNr++); // transfer unsigned int to string

            // now set the sampler to the correct texture unit
            shader.setInt(("material." + name + number).c_str(), i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }


private:
    void setupMesh() {
        // set up buffer data
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), 
                     &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                     &indices[0], GL_STATIC_DRAW);
        
        // set up attribute pointers
        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // texture coordinates
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    };

private:
    // ---------------------mesh data-----------------------
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    // ----------------------render data--------------------
    unsigned int VAO, VBO, EBO;

};