#pragma once

#include <string>
#include <memory>

#include <glad/glad.h>

class Texture2D {

public:
    Texture2D() {};

public:
    int width() { return width_; }
    int height() { return height_; }
    GLuint get_id() { return gl_texture_id_; }

public:
    static std::shared_ptr<Texture2D> load_texture(std::string image_file_path);
    
private:
    int width_{0};
    int height_{0};
    GLuint gl_texture_id_{0};
};