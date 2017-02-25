#pragma once
#include <glm/fwd.hpp>
#include <vulkan/vulkan.hpp>
#include "image2d.h"
#include "model_renderer.h"
#include "ui_renderer.h"

struct frame
{
    vk::Image image;
    vk::ImageView image_view;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer command_buffer;
    vk::Fence rendered_fence;
    image2d dsb;
    model_renderer mdl;
    ui_renderer ui;

    frame(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::DescriptorPool descriptor_pool, vk::Image image, vk::RenderPass render_pass, pipeline model_pipeline, pipeline ui_pipeline, model model, image2d font_image);
    void destroy(vk::Device device) const;
};
