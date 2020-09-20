#pragma once
#include <vulkan/vulkan.hpp>

struct image_with_memory
{
    uint32_t width;
    uint32_t height;
    vk::Format format;
    vk::UniqueDeviceMemory memory;
    vk::UniqueImage image;
    vk::ImageSubresourceRange sub_resource_range;

    image_with_memory(
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
    std::unique_ptr<image_with_memory> copy_from_host_to_device_for_shader_read(
        vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue) const;
    std::unique_ptr<image_with_memory> copy_from_device_to_host(vk::PhysicalDevice physical_device, vk::Device device,
                                                                vk::CommandPool command_pool, vk::Queue queue) const;
};

struct image_with_view
{
    std::unique_ptr<image_with_memory> iwm;
    vk::UniqueImageView image_view;

    image_with_view(vk::Device device, std::unique_ptr<image_with_memory> iwm);
};

std::unique_ptr<image_with_memory> load_r8g8b8a8_unorm_texture(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    uint32_t width,
    uint32_t height,
    const void* data
);
