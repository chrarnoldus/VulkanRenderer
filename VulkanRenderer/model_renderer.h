#pragma once

#include "renderer.h"
#include "data_types.h"
#include "model.h"
#include "pipeline.h"

class model_renderer : public renderer
{
    const model* mdl;
    const pipeline* model_pipeline;
    buffer uniform_buffer;
    vk::UniqueDescriptorSet descriptor_set;
    vk::Extent2D framebuffer_size;

public:
    model_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool,
                   vk::Extent2D framebuffer_size, const pipeline* model_pipeline, const model* mdl);
    void update(vk::Device device, model_uniform_data model_uniform_data) const override;

    void draw_outside_renderpass(vk::CommandBuffer command_buffer) const override;

    void draw(vk::CommandBuffer command_buffer) const override;
};
