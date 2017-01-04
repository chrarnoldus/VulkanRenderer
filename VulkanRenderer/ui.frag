#version 450 core

layout(set = 0, binding = 1) uniform sampler2D fontSampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 2) flat in vec4 clipRect;

layout(origin_upper_left) in vec4 gl_FragCoord;

layout(location = 0, index = 0) out vec4 fragColor;

void main()
{
    if (gl_FragCoord.x < clipRect.x || gl_FragCoord.x > clipRect.z || gl_FragCoord.y < clipRect.y || gl_FragCoord.y > clipRect.w) {
        discard;
    }
    fragColor = color * texture(fontSampler, uv.st);
}
