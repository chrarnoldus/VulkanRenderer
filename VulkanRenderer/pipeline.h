#pragma once
#include <vulkan/vulkan.h>
#include "buffer.h"

struct pipeline
{
    vk::Device device;
    vk::UniqueShaderModule vert_shader;
    vk::UniqueShaderModule frag_shader;
    std::vector<vk::Sampler> samplers; //owner
    vk::UniquePipelineLayout layout;
    vk::UniqueDescriptorSetLayout set_layout;
    vk::UniquePipeline pl;

    pipeline(vk::Device device, vk::UniqueShaderModule vert_shader, vk::UniqueShaderModule frag_shader, std::vector<vk::Sampler> samplers, vk::UniquePipelineLayout layout, vk::UniqueDescriptorSetLayout set_layout, vk::UniquePipeline pl);
    ~pipeline();
};

pipeline create_model_pipeline(vk::Device device, vk::RenderPass render_pass);
pipeline create_ui_pipeline(vk::Device device, vk::RenderPass render_pass);
