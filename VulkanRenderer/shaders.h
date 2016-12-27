#pragma once
#include <vulkan/vulkan.h>

extern const uint32_t WIDTH;
extern const uint32_t HEIGHT;

struct vertex
{
    float x;
    float y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
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
