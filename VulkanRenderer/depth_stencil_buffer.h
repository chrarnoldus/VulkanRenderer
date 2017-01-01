#pragma once
#include <vulkan/vulkan.hpp>

struct depth_stencil_buffer
{
    vk::DeviceMemory memory;
    vk::Image image;
    vk::ImageView image_view;

    depth_stencil_buffer(vk::PhysicalDevice physical_device, vk::Device device, uint32_t width, uint32_t height);
    void destroy(vk::Device device) const;
};
