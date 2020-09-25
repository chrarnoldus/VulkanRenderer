#pragma once
#include <vulkan/vulkan.hpp>

bool is_ray_tracing_supported(vk::PhysicalDevice physical_device);

class vulkan_context
{
public:
    vk::PhysicalDevice physical_device;
    vk::Device device;
    vk::Queue queue;
    vk::UniqueCommandPool command_pool;
    vk::UniqueRenderPass render_pass;
    vk::UniqueDescriptorPool descriptor_pool;
    bool is_ray_tracing_supported;

    vulkan_context(vk::PhysicalDevice physical_device, vk::Device device);
};
