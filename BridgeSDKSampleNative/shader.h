#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "ogl.h"  // Ensure ogl.h is included for ogl:: function pointers

class Shader {
public:
    GLuint ID;

    // Constructor that builds the shader program from two file paths
    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode = loadShaderCode(vertexPath);
        std::string fragmentCode = loadShaderCode(fragmentPath);
        compile(vertexCode.c_str(), fragmentCode.c_str());
    }

    // Activate the shader program
    void use() const { ogl::glUseProgram(ID); }

    // Utility functions for setting uniform values in the shader
    void setFloat(const std::string &name, float value) const {
        ogl::glUniform1f(ogl::glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setInt(const std::string &name, int value) const {
        ogl::glUniform1i(ogl::glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setMat4(const std::string &name, const float* mat) const {
        ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat);
    }

private:
    // Load the shader code from a file
    std::string loadShaderCode(const char* path) {
        std::ifstream shaderFile(path);
        if (!shaderFile) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << path << "\n";
            return "";
        }
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        return shaderStream.str();
    }

    // Compile and link the shaders
    void compile(const char* vertexCode, const char* fragmentCode) {
        // Compile vertex shader
        GLuint vertex = ogl::glCreateShader(GL_VERTEX_SHADER);
        ogl::glShaderSource(vertex, 1, &vertexCode, nullptr);
        ogl::glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Compile fragment shader
        GLuint fragment = ogl::glCreateShader(GL_FRAGMENT_SHADER);
        ogl::glShaderSource(fragment, 1, &fragmentCode, nullptr);
        ogl::glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Link shaders into a program
        ID = ogl::glCreateProgram();
        ogl::glAttachShader(ID, vertex);
        ogl::glAttachShader(ID, fragment);
        ogl::glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        ogl::glDeleteShader(vertex);
        ogl::glDeleteShader(fragment);
    }

    // Check and print compile/link errors
    void checkCompileErrors(GLuint shader, const std::string& type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            ogl::glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                ogl::glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n";
            }
        } else {
            ogl::glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                ogl::glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n";
            }
        }
    }
};

#endif
