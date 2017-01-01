#pragma once
#include <glm/fwd.hpp>
#include <vulkan/vulkan.hpp>
#include "buffer.h"
#include "pipeline.h"
#include "model.h"
#include "depth_stencil_buffer.h"

struct uniform_data
{
    glm::mat4 transform;
};

struct frame
{
    vk::Image image;
    vk::ImageView image_view;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer command_buffer;
    vk::Fence rendered_fence;
    buffer uniform_buffer;
    depth_stencil_buffer dsb;

    frame(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::DescriptorPool descriptor_pool, vk::Image image, vk::RenderPass render_pass, pipeline pipeline, model model);
    void destroy(vk::Device device) const;
};
