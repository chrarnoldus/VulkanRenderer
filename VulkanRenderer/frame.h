#pragma once
#include <glm/fwd.hpp>
#include <vulkan/vulkan.hpp>
#include "image_with_view.h"
#include "renderer.h"

class frame
{
    vk::Image image;
    vk::ImageView image_view;
    vk::Framebuffer framebuffer;
    image_with_view dsb;
    std::vector<renderer*> renderers;

public:
    vk::CommandBuffer command_buffer;
    vk::Fence rendered_fence;

    frame(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::DescriptorPool descriptor_pool, vk::Image image, vk::Format format, vk::RenderPass render_pass, std::vector<renderer*> renderers);
    void update(vk::Device device, model_uniform_data model_uniform_data) const;
    void destroy(vk::Device device) const;
};
