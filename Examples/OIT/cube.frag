#version 450

layout (location = 0) in vec3 FragPos;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec4 Color;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = Color;
}