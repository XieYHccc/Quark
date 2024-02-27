#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Shader
{   
public:
    Shader();
    // constructor reads and builds the shader
    Shader(std::string vertexPath, std::string fragmentPath);
public:

    void load_shader(std::string vertexPath, std::string fragmentPath);
    // use/activate the shader
    bool use() const;
    // utility uniform functions
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, glm::mat4 mat) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec3(const std::string &name, const glm::vec3& vec);

    unsigned int get_id() const { return ID;}

private:
    // utility function for checking shader compilation/linking errors
    // ---------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type);

private:
    unsigned int ID;
};
