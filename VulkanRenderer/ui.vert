#version 450 core

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec4 vertexColor;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;

void main()
{
    // TODO no hardcoding viewport size
    gl_Position = vec4((vertexPosition.x - 512.0) / 512.0, (vertexPosition.y - 384.0) / 384.0, 0.0, 1.0);
    color = vertexColor;
    uv = vertexUV;
}
