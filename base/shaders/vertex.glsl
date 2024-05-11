#version 450

#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color; 

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec3 player_pos;
layout (location = 2) out vec4 world_pos;

layout(std430, buffer_reference, buffer_reference_align = 8) readonly buffer Values
{
    float numbers[];
};

layout (std430, push_constant) uniform UniformObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec3 playerpos;
    Values values;
} ubo;

void main() {
    vec3 light_dir = vec3(0, -1, -0.5);
    
    world_pos = ubo.model * vec4(position, 1.0);
    gl_Position = ubo.proj * ubo.view * world_pos;
    gl_Position.y *= -1;
    
    player_pos = ubo.playerpos;
    
    float val = max(dot(light_dir, normal), 0.0);
    fragColor = vec4(val + 0.2) * color * ubo.values.numbers[0];
    fragColor.w = 1;
}