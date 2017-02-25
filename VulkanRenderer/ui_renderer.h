#pragma once
#include <vulkan/vulkan.hpp>

#include "renderer.h"
#include "image2d.h"
#include "buffer.h"
#include "pipeline.h"

class ui_renderer : public renderer
{
    buffer vertex_buffer;
    buffer index_buffer;
    buffer indirect_buffer;
    buffer uniform_buffer;
    pipeline ui_pipeline;
    image2d font_image;
    vk::DescriptorSet descriptor_set;

public:
    ui_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool, pipeline ui_pipeline, image2d font_image);
    void update(vk::Device device, model_uniform_data model_uniform_data) const override;
    void draw(vk::CommandBuffer command_buffer) const override;
    void destroy(vk::Device device) const override;
};
