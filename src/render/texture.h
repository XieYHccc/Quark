#pragma once

#include <string>

#include <glad/glad.h>

class Texture {

public:
    Texture() {};

public:
    int width(){return width_;}
    int height(){return height_;}
    GLuint get_id(){ return gl_texture_id_;}

public:
    static Texture* load_texture(std::string image_file_path);
    
private:
    int width_;
    int height_;
    GLenum gl_texture_format_;
    GLuint gl_texture_id_{0};
};