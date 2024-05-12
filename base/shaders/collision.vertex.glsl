#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color; 

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec3 player_pos;
layout (location = 2) out vec4 world_pos;

layout (binding = 0) uniform UniformObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec3 playerpos;
} ubo;

void main() {    
    world_pos = ubo.model * vec4(position, 1.0);
    gl_Position = ubo.proj * ubo.view * world_pos;
    gl_Position.y *= -1;
    
    player_pos = ubo.playerpos;
    
    fragColor = color;
}