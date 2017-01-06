#pragma once
#include <vulkan/vulkan.hpp>

struct image2d
{
    uint32_t width;
    uint32_t height;
    vk::Format format;
    vk::DeviceMemory memory;
    vk::Image image;
    vk::ImageView image_view;
    vk::ImageSubresourceRange sub_resource_range;

    image2d(
        vk::PhysicalDevice physical_device,
        vk::Device device,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageUsageFlags usage_flags,
        vk::ImageTiling image_tiling,
        vk::ImageLayout initial_layout,
        vk::MemoryPropertyFlags memory_flags,
        vk::ImageAspectFlags aspect_flags
    );
    image2d copy_from_host_to_device_for_shader_read(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue) const;
    void destroy(vk::Device device) const;
};

image2d load_r8g8b8a8_unorm_texture(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    uint32_t width,
    uint32_t height,
    const void* data
);
