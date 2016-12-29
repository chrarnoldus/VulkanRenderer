#pragma once
#include <vulkan/vulkan.hpp>
#include "buffer.h"
#include "pipeline.h"

struct frame
{
    vk::Image image;
    vk::ImageView image_view;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer command_buffer;
    vk::Semaphore submitted_semaphore;

    frame(vk::Device device, vk::CommandPool command_pool, vk::Image image, vk::RenderPass render_pass, pipeline pipeline, buffer vertex_buffer);
    void destroy(vk::Device device) const;
};
