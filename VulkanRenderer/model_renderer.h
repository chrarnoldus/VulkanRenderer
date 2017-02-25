#pragma once

#include "data_types.h"
#include "model.h"
#include "pipeline.h"

class model_renderer
{
    model mdl;
    pipeline model_pipeline;
    buffer uniform_buffer;
    vk::DescriptorSet descriptor_set;

public:
    model_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool, pipeline model_pipeline, model mdl);
    void update(vk::Device device, model_uniform_data model_uniform_data);
    void draw(vk::CommandBuffer command_buffer) const;
    void destroy(vk::Device device) const;
};
