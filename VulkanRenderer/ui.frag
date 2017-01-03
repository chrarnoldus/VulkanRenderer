#version 450 core

layout(set = 0, binding = 0) uniform sampler2D fontSampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;

layout(location = 0, index = 0) out vec4 fragColor;

void main()
{
    fragColor = color * texture(fontSampler, uv.st);
}
