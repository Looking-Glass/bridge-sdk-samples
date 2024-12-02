#version 330 core

in vec2 TexCoords;     // Input texture coordinates from vertex shader
in vec3 Normal;     // Input texture coordinates from vertex shader

uniform sampler2D textureSampler; // Texture sampler

out vec4 FragColor;

void main() {
    // Sample the texture and output the color
    FragColor = texture(textureSampler, TexCoords);
}
