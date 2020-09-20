#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "buffer.h"

struct model
{
    uint32_t index_count;
    uint32_t vertex_count;
    std::unique_ptr<buffer> vertex_buffer;
    std::unique_ptr<buffer> index_buffer;

    model(uint32_t vertex_count, uint32_t index_count, std::unique_ptr<buffer> vertex_buffer,
          std::unique_ptr<buffer> index_buffer);
    void draw(vk::CommandBuffer command_buffer) const;
};

model read_model(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue,
                 const std::string& path);
