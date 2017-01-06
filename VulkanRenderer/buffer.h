#pragma once
#include <vulkan/vulkan.hpp>

#define HOST_VISIBLE_AND_COHERENT (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)

struct buffer
{
    vk::DeviceSize size;
    vk::DeviceMemory memory;
    vk::Buffer buf;

    buffer(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags, vk::DeviceSize size);
    void update(vk::Device device, void* data) const;
    buffer copy_from_host_to_device_for_vertex_input(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags new_usage_flags, vk::CommandPool command_pool, vk::Queue queue) const;
    void destroy(vk::Device device) const;
};
