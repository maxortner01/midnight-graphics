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
    mat4 models[];
};

layout(std430, buffer_reference, buffer_reference_align = 8) readonly buffer FrameInfo
{
    mat4 proj, view;
    vec3 player_pos;
};

layout (std430, push_constant) uniform Constants
{
    uint index;
    Values transforms;
    FrameInfo frame_info;
} constants;

void main() {
    vec3 light_dir = vec3(0, -1, -0.5);
    
    world_pos = constants.transforms.models[constants.index] * vec4(position, 1.0);
    gl_Position = constants.frame_info.proj * constants.frame_info.view * world_pos;
    gl_Position.y *= -1;
    
    player_pos = constants.frame_info.player_pos;
    
    float val = max(dot(light_dir, normal), 0.0);
    fragColor = vec4(val + 0.2) * color;
    fragColor.w = 1;
}