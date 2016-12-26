#pragma once
#include <vulkan/vulkan.h>

extern const uint32_t WIDTH;
extern const uint32_t HEIGHT;

struct vertex
{
    float position[2];
    uint8_t color[3];
};

struct shader_info
{
    vk::ShaderModule vert_shader;
    vk::ShaderModule frag_shader;
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;

    void destroy(vk::Device device);
};

shader_info create_pipeline(vk::Device device, vk::RenderPass render_pass, const char* vert_shader_file_name, const char* frag_shader_file_name);
