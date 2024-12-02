#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <ogl.h>  // Include the ogl namespace
#include "OBJ_Loader.h"  // Include the OBJ Loader library
#include "texture.h"     // Include the Texture class
#include "LKGCamera.h"

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
    std::unique_ptr<Texture> texture;

    glm::vec3 position = glm::vec3(0.0f);   // Position of the mesh
    glm::vec3 scale = glm::vec3(1.0f);      // Scale of the mesh
    glm::vec3 rotation = glm::vec3(0.0f);  // Rotation (Euler angles in radians)

    // Rule of Five: Explicitly define move constructor and move assignment operator
    Mesh(Mesh&& other) noexcept
        : vertices(std::move(other.vertices)),
          indices(std::move(other.indices)),
          VAO(other.VAO),
          texture(std::move(other.texture)),
          position(other.position),
          scale(other.scale),
          rotation(other.rotation) {
        other.VAO = 0; // Reset other's VAO to a safe state
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            VAO = other.VAO;
            texture = std::move(other.texture);
            position = other.position;
            scale = other.scale;
            rotation = other.rotation;
            other.VAO = 0; // Reset other's VAO to a safe state
        }
        return *this;
    }

    // Delete copy constructor and copy assignment operator
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Existing constructor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, const std::string& texturePath = "")
        : vertices(std::move(vertices)), indices(std::move(indices)) {
        if (!texturePath.empty()) {
            texture = std::make_unique<Texture>(texturePath);
        }
        setupMesh();
    }

    Matrix4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z
        model = glm::scale(model, scale);

        // Convert glm::mat4 to Matrix4
        return Matrix4(glm::value_ptr(model));
    }

    static Mesh loadObj(const std::string& filepath, const std::string& textureOverride = "") {
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
            glm::vec2 texCoords = glm::vec2(v.TextureCoordinate.X, 1.0f - v.TextureCoordinate.Y);
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

        // Use the override texture if provided; otherwise, use the one from the OBJ file
        std::string texturePath = !textureOverride.empty() ? textureOverride : curMesh.MeshMaterial.map_Kd;

        return Mesh(vertices, indices, texturePath);
    }

    static std::vector<Mesh> loadObjs(const std::string& filepath, const std::string& textureOverride = "") {
        objl::Loader Loader;

        // Attempt to load the OBJ file
        if (!Loader.LoadFile(filepath)) {
            throw std::runtime_error("Failed to load OBJ file: " + filepath);
        }

        std::vector<Mesh> meshes;

        for (const auto& curMesh : Loader.LoadedMeshes) {
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
                glm::vec2 texCoords = glm::vec2(v.TextureCoordinate.X, 1.0f - v.TextureCoordinate.Y);

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

            // Use the override texture if provided; otherwise, use the one from the OBJ file
            std::string texturePath = !textureOverride.empty() ? textureOverride : curMesh.MeshMaterial.map_Kd;

            meshes.emplace_back(vertices, indices, texturePath);
        }

        return meshes;
    }


    void bindTexture() const
    {
        // Bind the texture if available
        if (texture) {
            ogl::glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
            texture->bind();                  // Bind the texture
        }
    }

    void draw() const 
    {
        // Bind the VAO and render the mesh
        ogl::glBindVertexArray(VAO);
        ogl::glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        ogl::glBindVertexArray(0);

        // Unbind the texture (optional cleanup step)
        ogl::glBindTexture(GL_TEXTURE_2D, 0);
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
