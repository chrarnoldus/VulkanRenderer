#pragma once
#include <vulkan/vulkan.h>
#include "buffer.h"

struct vertex
{
    float x;
    float y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct pipeline
{
    vk::ShaderModule vert_shader;
    vk::ShaderModule frag_shader;
    vk::PipelineLayout layout;
    vk::DescriptorSetLayout set_layout;
    vk::Pipeline pl;

    pipeline(vk::Device device, vk::RenderPass render_pass, const char* vert_shader_file_name, const char* frag_shader_file_name);
    void destroy(vk::Device device) const;
};

