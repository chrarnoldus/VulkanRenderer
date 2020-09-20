#pragma once
#include "image_with_view.h"
#include "pipeline.h"
#include "ray_tracing_model.h"
#include "renderer.h"

class ray_tracing_renderer : public renderer
{
    const ray_tracing_model* model;
    const buffer* shader_binding_table;
    const pipeline* ray_tracing_pipeline;
    const pipeline* textured_quad_pipeline;
    buffer uniform_buffer;
    buffer textured_quad; //could be shared
    vk::UniqueDescriptorSet ray_tracing_descriptor_set;
    vk::UniqueDescriptorSet textured_quad_descriptor_set;
    image_with_view image;

    void initialize_ray_tracing_descriptor_set(vk::Device device);

public:
    ray_tracing_renderer(vk::PhysicalDevice physical_device, vk::Device device,
                         vk::DescriptorPool descriptor_pool,
                         const pipeline* ray_tracing_pipeline, const pipeline* textured_quad_pipeline,
                         const buffer* shader_binding_table,
                         const ray_tracing_model* model);
    void update(vk::Device device, model_uniform_data model_uniform_data) const override;
    void draw_outside_renderpass(vk::CommandBuffer command_buffer) const override;
    void draw(vk::CommandBuffer command_buffer) const override;
};
