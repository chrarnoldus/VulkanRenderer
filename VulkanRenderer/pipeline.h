#pragma once
#include <vulkan/vulkan.h>
#include "buffer.h"

struct pipeline
{
    vk::ShaderModule vert_shader;
    vk::ShaderModule frag_shader;
    std::vector<vk::Sampler> samplers;
    vk::PipelineLayout layout;
    vk::DescriptorSetLayout set_layout;
    vk::Pipeline pl;

    pipeline(vk::ShaderModule vert_shader, vk::ShaderModule frag_shader, std::vector<vk::Sampler> samplers, vk::PipelineLayout layout, vk::DescriptorSetLayout set_layout, vk::Pipeline pl);
    void destroy(vk::Device device) const;
};

pipeline create_model_pipeline(vk::Device device, vk::RenderPass render_pass);
pipeline create_ui_pipeline(vk::Device device, vk::RenderPass render_pass);
