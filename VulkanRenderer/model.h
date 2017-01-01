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
    void destroy(vk::Device device) const;
};

model read_model(vk::PhysicalDevice physical_device, vk::Device device, const std::string& path);
