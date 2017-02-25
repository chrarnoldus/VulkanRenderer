#pragma once
#include <vulkan/vulkan.hpp>

#include "image2d.h"
#include "buffer.h"
#include "pipeline.h"

class ui_renderer
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
    void update(vk::Device device) const;
    void draw(vk::CommandBuffer command_buffer) const;
    void destroy(vk::Device device) const;
};
