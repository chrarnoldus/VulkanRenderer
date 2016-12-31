#pragma once
#include <vulkan/vulkan.hpp>
#include "buffer.h"
#include "pipeline.h"
#include "frame.h"

struct vulkanapp
{
    vk::Queue queue;
    buffer mesh;
    vk::RenderPass render_pass;
    pipeline pl;
    vk::CommandPool command_pool;
    vk::DescriptorPool descriptor_pool;
    vk::Semaphore acquired_semaphore;
    vk::Semaphore rendered_semaphore;
    std::vector<frame> frames;
    vk::SwapchainKHR swapchain;

    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface);
    void update(vk::Device device, double timeInSeconds) const;
    void destroy(vk::Device device) const;
};
