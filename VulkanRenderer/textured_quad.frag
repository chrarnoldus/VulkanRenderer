#version 460

layout(binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 uv;

layout(location = 0, index = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(textureSampler, uv.xy);
}
