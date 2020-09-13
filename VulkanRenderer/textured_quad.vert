#version 460

layout(location = 0) in vec2 position;

layout(location = 0) out vec2 uv;

void main()
{
    gl_Position = vec4(position, 1.0, 1.0);
    uv = (position - 1.0) / -2.0;
}
