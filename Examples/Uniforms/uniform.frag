#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBlock {
    float t;
} ub;


void main() {
    outColor = vec4(fragColor * ub.t, 1.0);
}