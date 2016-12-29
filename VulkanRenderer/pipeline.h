#pragma once
#include <vulkan/vulkan.h>
#include "buffer.h"

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

struct pipeline
{
    vk::ShaderModule vert_shader;
    vk::ShaderModule frag_shader;
    vk::PipelineLayout layout;
    vk::DescriptorSetLayout set_layout;
    vk::DescriptorPool descriptor_pool;
    std::vector<vk::DescriptorSet> descriptor_sets;
    vk::Pipeline pl;

    pipeline(vk::Device device, vk::RenderPass render_pass, buffer uniform_buffer, const char* vert_shader_file_name, const char* frag_shader_file_name);
    void destroy(vk::Device device) const;
};

