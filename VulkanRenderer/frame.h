#pragma once
#include <glm/fwd.hpp>
#include <vulkan/vulkan.hpp>
#include "image_with_view.h"
#include "renderer.h"

class frame
{
    vk::Device device;
    vk::Image image;
    vk::UniqueImageView image_view;
    vk::UniqueFramebuffer framebuffer;
    image_with_view dsb;
    std::vector<std::unique_ptr<renderer>> renderers;

public:
    vk::CommandBuffer command_buffer;
    vk::UniqueFence rendered_fence;

    frame(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool,
          vk::DescriptorPool descriptor_pool, vk::Image image, vk::Format format, vk::RenderPass render_pass,
          std::vector<std::unique_ptr<renderer>> renderers);
    void update(model_uniform_data model_uniform_data) const;
};
