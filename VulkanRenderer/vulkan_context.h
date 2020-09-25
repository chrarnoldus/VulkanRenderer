#pragma once
#include <vulkan/vulkan.hpp>

class vulkan_context
{
public:
    vk::PhysicalDevice physical_device;
    vk::Device device;
    vk::Queue queue;
    vk::UniqueCommandPool command_pool;
    vk::UniqueRenderPass render_pass;
    vk::UniqueDescriptorPool descriptor_pool;

    vulkan_context(vk::PhysicalDevice physical_device, vk::Device device);
};