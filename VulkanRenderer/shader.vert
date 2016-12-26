#version 450

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec3 vertexColor;

out vec3 color;

void main()
{
    gl_Position = vec4(vertexPosition, 0.0, 1.0);
    color = vertexColor;
}
