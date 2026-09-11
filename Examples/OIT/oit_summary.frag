#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInputMS inputColor;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 previousColor = subpassLoad(inputColor, gl_SampleID);
    outColor = previousColor;
}