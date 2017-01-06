#pragma once
#include <vulkan/vulkan.hpp>
#include "input_state.h"
#include "pipeline.h"
#include "frame.h"
#include "model.h"

struct vulkanapp
{
    vk::Queue queue;
    vk::CommandPool command_pool;
    model mdl;
    vk::RenderPass render_pass;
    pipeline model_pipeline;
    pipeline ui_pipeline;
    vk::DescriptorPool descriptor_pool;
    vk::Semaphore acquired_semaphore;
    vk::Semaphore rendered_semaphore;
    std::vector<frame> frames;
    vk::SwapchainKHR swapchain;
    image2d font_image;
    glm::quat trackball_rotation;
    float camera_distance;

    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, const std::string& model_path);
    void update(vk::Device device, const input_state& input);
    void destroy(vk::Device device) const;
};
