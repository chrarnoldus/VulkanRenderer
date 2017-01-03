#pragma once
#include <glm/fwd.hpp>
#include <vulkan/vulkan.hpp>
#include "buffer.h"
#include "pipeline.h"
#include "model.h"
#include "image2d.h"
#include "ui_renderer.h"

struct frame
{
    vk::Image image;
    vk::ImageView image_view;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer command_buffer;
    vk::Fence rendered_fence;
    buffer uniform_buffer;
    image2d dsb;
    ui_renderer ui;

    frame(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::DescriptorPool descriptor_pool, vk::Image image, vk::RenderPass render_pass, pipeline pipeline, model model);
    void destroy(vk::Device device) const;
};
