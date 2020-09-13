#version 460
#extension GL_NV_ray_tracing : enable

layout(location = 0) rayPayloadInNV vec3 color;

void main()
{
    color = vec3(0.0, 0.0, 1.0);
}
