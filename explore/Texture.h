#pragma once

#include <string>
#include <iostream>

#include <glad/glad.h>
#include <stb_image.h>

class Texture {
public:
    Texture() : id_(0), file_path_("") {}

public:
    bool load_texture(const char* file_path) {
        glGenTextures(1, &id_);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(file_path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, id_);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            return 1;
        }
        else {
            std::cout << "Texture failed to load at path: " << file_path << std::endl;
            stbi_image_free(data);
            return 0;
        }
    }

public:
	unsigned int get_id() { return id_; }
    std::string get_filepath() { return file_path_; }
    
private:
	unsigned int id_;
	std::string file_path_;
};