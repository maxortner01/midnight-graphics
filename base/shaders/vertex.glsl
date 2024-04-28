#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color; 

layout (location = 0) out vec4 fragColor;

layout (binding = 0) uniform UniformObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.model * vec4(position.xy, -1, 1.0);
    fragColor = color;
}