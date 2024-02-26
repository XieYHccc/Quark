#include "texture.h"

#include <iostream>
#include <tiny_obj_loader.h>
#include <stb_image.h>

Texture* Texture::load_texture(std::string image_file_path) {

    Texture* texture=new Texture();

    int width, height, nrComponents;
    unsigned char* data = stbi_load(image_file_path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        texture->width_ = width;
        texture->height_ = height;
        texture->gl_texture_format_ =format;
        
        glGenTextures(1, &texture->gl_texture_id_);
        glBindTexture(GL_TEXTURE_2D, texture->gl_texture_id_);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << image_file_path << std::endl;
        stbi_image_free(data);
    }

    return texture;
}