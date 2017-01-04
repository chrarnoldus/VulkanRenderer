#pragma once
#include <vulkan/vulkan.hpp>
#include "buffer.h"
#include "pipeline.h"
#include "frame.h"
#include "model.h"

struct vulkanapp
{
    vk::Queue queue;
    model mdl;
    vk::RenderPass render_pass;
    pipeline model_pipeline;
    pipeline ui_pipeline;
    vk::CommandPool command_pool;
    vk::DescriptorPool descriptor_pool;
    vk::Semaphore acquired_semaphore;
    vk::Semaphore rendered_semaphore;
    std::vector<frame> frames;
    vk::SwapchainKHR swapchain;
    image2d font_image;

    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, model mdl);
    void update(vk::Device device, float camera_distance, const glm::mat4& rotation) const;
    void destroy(vk::Device device) const;
};
