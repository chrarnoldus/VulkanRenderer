#version 450 core

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec4 vertexColor;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;

void main()
{
    gl_Position = vec4(vertexPosition, 0.0, 1.0);
    color = vertexColor;
    uv = vertexUV;
}
