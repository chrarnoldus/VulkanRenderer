#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "buffer.h"

struct model
{
    uint32_t index_count;
    buffer vertex_buffer;
    buffer index_buffer;

    model(uint32_t index_count, buffer vertex_buffer, buffer index_buffer);
    void draw(vk::CommandBuffer command_buffer) const;
    void destroy(vk::Device device) const;
};

model read_model(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue, const std::string& path);
