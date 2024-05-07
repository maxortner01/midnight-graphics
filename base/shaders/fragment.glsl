#version 450

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec3 player_pos;
layout (location = 2) in vec4 world_pos;

layout(location = 0) out vec4 outColor;

float getScalar(float x, float r)
{
    //return 1.0 - (x / r) * exp(x - r);
    return pow(1 - x / r, 0.5);
}

void main() {
    float r = length(world_pos.xyz - player_pos);
    outColor = fragColor * getScalar(r, 20);
    outColor.a = 1;
}