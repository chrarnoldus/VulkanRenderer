#version 450

layout(set = 0, binding = 0, std140) uniform ub
{
    mat4 projection;
    mat4 modelView;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexColor;

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 color;

void main()
{
    gl_Position = projection * modelView * vec4(vertexPosition, 1.0);
    position = vec3(modelView * vec4(vertexPosition, 1.0));
    normal = vec3(modelView * vec4(vertexNormal, 0.0));
    color = vertexColor;
}
