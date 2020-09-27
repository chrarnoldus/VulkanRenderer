#pragma once
#include "frame_set.h"
#include "pipeline.h"
#include "ray_tracing_model.h"

class ray_tracer
{
    const pipeline* ui_pipeline;
    const image_with_view* font_image;

public:
    ray_tracing_model ray_tracing_model;
    pipeline textured_quad_pipeline;
    pipeline model_pipeline;
    std::unique_ptr<buffer> shader_binding_table;
    frame_set frame_set;
    ray_tracer(
        const vulkan_context& context,
        const std::vector<vk::Image>& images,
        vk::Extent2D framebuffer_size,
        const model* model,
        const pipeline* ui_pipeline,
        const image_with_view* font_image);
    void recreate_swapchain(const vulkan_context& context, vk::Extent2D framebuffer_size,
                            const std::vector<vk::Image>& images);
};
