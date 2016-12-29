#pragma once
#include <vulkan/vulkan.hpp>

struct buffer
{
    vk::DeviceSize allocation_size;
    vk::DeviceMemory memory;
    vk::Buffer buf;

    buffer(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags usage_flags, vk::DeviceSize allocation_size);
    void update(vk::Device device, void *data) const;
    void destroy(vk::Device device) const;
};
