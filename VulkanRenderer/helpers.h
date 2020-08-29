#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

vk::UniqueRenderPass create_render_pass(vk::Device device, vk::Format color_format, vk::ImageLayout final_layout);
vk::UniqueDescriptorPool create_descriptor_pool(vk::Device device);

void render_to_image(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    const std::string& model_path,
    const std::string& image_path,
    const glm::vec3& camera_position,
    const glm::vec3& camera_up
);
