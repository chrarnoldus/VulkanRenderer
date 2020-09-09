#pragma once
#include <vulkan/vulkan.h>
#include "buffer.h"

#define GROUP_COUNT 3u
#define RAYGEN_SHADER_INDEX 0u
#define CLOSEST_HIT_SHADER_INDEX 1u
#define MISS_SHADER_INDEX 2u

struct pipeline
{
    vk::Device device;
    vk::UniqueShaderModule vert_shader;
    vk::UniqueShaderModule frag_shader;
    std::vector<vk::ShaderModule> shader_modules; //owner
    std::vector<vk::Sampler> samplers; //owner
    vk::UniquePipelineLayout layout;
    vk::UniqueDescriptorSetLayout set_layout;
    vk::UniquePipeline pl;

    pipeline(vk::Device device, std::vector<vk::ShaderModule> shader_modules, std::vector<vk::Sampler> samplers, vk::UniquePipelineLayout layout, vk::UniqueDescriptorSetLayout set_layout, vk::UniquePipeline pl);
    ~pipeline();
};

pipeline create_model_pipeline(vk::Device device, vk::RenderPass render_pass);
pipeline create_ui_pipeline(vk::Device device, vk::RenderPass render_pass);
pipeline create_ray_tracing_pipeline(vk::Device device);
