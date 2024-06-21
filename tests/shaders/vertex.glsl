#version 450

#extension GL_EXT_buffer_reference : require

layout(std430, buffer_reference, buffer_reference_align = 8) readonly buffer Vertices
{
    vec2 vertices[];
};

layout (std430, push_constant) uniform Constants
{
    Vertices data;
} constants;

void main() {
    gl_Position = vec4(constants.data.vertices[gl_VertexIndex].xy, 0.0, 1.0);
}