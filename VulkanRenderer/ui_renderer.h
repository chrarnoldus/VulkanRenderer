#pragma once
#include <vulkan/vulkan.hpp>

#include "renderer.h"
#include "image_with_view.h"
#include "buffer.h"
#include "pipeline.h"

class ui_renderer : public renderer
{
    buffer vertex_buffer;
    buffer index_buffer;
    buffer indirect_buffer;
    buffer uniform_buffer;
    const pipeline* ui_pipeline;
    const image_with_view* font_image;
    vk::UniqueDescriptorSet descriptor_set;
    vk::Extent2D framebuffer_size;

public:
    ui_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool,
        vk::Extent2D framebuffer_size, const pipeline* ui_pipeline, const image_with_view* font_image);
    void update(vk::Device device, model_uniform_data model_uniform_data) const override;

    void draw_outside_renderpass(vk::CommandBuffer command_buffer) const override;

    void draw(vk::CommandBuffer command_buffer) const override;
};
