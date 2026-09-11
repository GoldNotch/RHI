#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D textureAtlas;

void main() {
	vec4 texel = texture(textureAtlas, fragUV);
    outColor = vec4(texel.rgb, 1.0);
}