#version 450 core

layout(set = 0, binding = 0, std140) uniform ub
{
    float screenWidth;
    float screenHeight;
    vec4 clipRects[64];
};

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec4 vertexColor;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;
layout(location = 2) flat out vec4 clipRect;

void main()
{
    float halfScreenWidth = screenWidth / 2.0;
    float halfScreenHeigth = screenHeight / 2.0;
    gl_Position = vec4((vertexPosition.x - halfScreenWidth) / halfScreenWidth, (vertexPosition.y - halfScreenHeigth) / halfScreenHeigth, 0.0, 1.0);
    color = vertexColor;
    uv = vertexUV;
    clipRect = clipRects[gl_InstanceIndex];
}
