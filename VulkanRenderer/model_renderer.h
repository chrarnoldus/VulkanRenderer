#pragma once

#include "renderer.h"
#include "data_types.h"
#include "model.h"
#include "pipeline.h"

class model_renderer : public renderer
{
    const model* mdl;
    pipeline model_pipeline;
    buffer uniform_buffer;
    vk::DescriptorSet descriptor_set;

public:
    model_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool, pipeline model_pipeline, const model* mdl);
    void update(vk::Device device, model_uniform_data model_uniform_data) const override;
    void draw(vk::CommandBuffer command_buffer) const override;
};
