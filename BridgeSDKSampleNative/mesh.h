#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <ogl.h>  // Include the ogl namespace
#include "OBJ_Loader.h"  // Include the OBJ Loader library

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) 
        : vertices(vertices), indices(indices) {
        setupMesh();
    }

    // Static function to generate a cube mesh
    static Mesh createCube() {
        std::vector<Vertex> vertices = {
            // Front face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

            // Back face
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

            // Left face
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

            // Right face
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

            // Bottom face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

            // Top face
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
        };

        std::vector<unsigned int> indices = {
            0, 1, 2, 2, 3, 0,        // Front face
            4, 5, 6, 6, 7, 4,        // Back face
            8, 9, 10, 10, 11, 8,     // Left face
            12, 13, 14, 14, 15, 12,  // Right face
            16, 17, 18, 18, 19, 16,  // Bottom face
            20, 21, 22, 22, 23, 20   // Top face
        };

        return Mesh(vertices, indices);
    }


    static Mesh loadObj(const std::string& filepath) {
        objl::Loader Loader;

        // Attempt to load the OBJ file
        if (!Loader.LoadFile(filepath)) {
            throw std::runtime_error("Failed to load OBJ file: " + filepath);
        }

        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        glm::vec3 minPos(FLT_MAX, FLT_MAX, FLT_MAX);
        glm::vec3 maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (const auto& v : curMesh.Vertices) {
            glm::vec3 position(v.Position.X, v.Position.Y, v.Position.Z);
            minPos = glm::min(minPos, position);
            maxPos = glm::max(maxPos, position);
        }

        glm::vec3 center = (minPos + maxPos) / 2.0f;
        float maxDimension = std::max(std::max(maxPos.x - minPos.x, maxPos.y - minPos.y), maxPos.z - minPos.z);
        float scale = (1.0f / maxDimension) * 6.0f;

        for (const auto& v : curMesh.Vertices) {
            glm::vec3 position(v.Position.X, v.Position.Y, v.Position.Z);
            position = (position - center) * scale;

            glm::vec3 normal = glm::vec3(v.Normal.X, v.Normal.Y, v.Normal.Z);
            glm::vec2 texCoords = glm::vec2(v.TextureCoordinate.X, v.TextureCoordinate.Y);

            vertices.push_back({ position, normal, texCoords });
        }

        for (size_t i = 0; i < curMesh.Indices.size(); i += 3) {
            unsigned int i1 = curMesh.Indices[i];
            unsigned int i2 = curMesh.Indices[i + 1];
            unsigned int i3 = curMesh.Indices[i + 2];

            indices.push_back(i1);
            indices.push_back(i3);  // Ensure CCW winding
            indices.push_back(i2);
        }

        return Mesh(vertices, indices);
    }

    void draw() const {
        ogl::glBindVertexArray(VAO);
        ogl::glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        ogl::glBindVertexArray(0);
    }

private:
    GLuint VBO, EBO;

    void setupMesh() {
        ogl::glGenVertexArrays(1, &VAO);
        ogl::glGenBuffers(1, &VBO);
        ogl::glGenBuffers(1, &EBO);

        ogl::glBindVertexArray(VAO);

        ogl::glBindBuffer(GL_ARRAY_BUFFER, VBO);
        ogl::glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        ogl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        ogl::glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        ogl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        ogl::glEnableVertexAttribArray(0);
        
        ogl::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        ogl::glEnableVertexAttribArray(1);

        ogl::glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
        ogl::glEnableVertexAttribArray(2);

        ogl::glBindVertexArray(0);
    }
};

#endif
