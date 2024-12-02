#ifndef TEXTURE_H
#define TEXTURE_H

#include "ogl.h"
#include <string>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Include stb_image for image loading

class Texture {
public:
    GLuint ID;
    int width, height;

    Texture(const std::string& path) {
        ogl::glGenTextures(1, &ID);
        ogl::glBindTexture(GL_TEXTURE_2D, ID);

        // Set texture wrapping and filtering options
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load image data with stb_image
        int channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (data) {
            GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
            ogl::glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            ogl::glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        stbi_image_free(data);
    }

    void bind() const { ogl::glBindTexture(GL_TEXTURE_2D, ID); }
};

#endif
