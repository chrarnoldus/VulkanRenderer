#pragma once
#include <vulkan/vulkan.hpp>

#include "buffer.h"

class ui_renderer
{
    buffer vertex_buffer;
    buffer index_buffer;
    buffer indirect_buffer;

public:
    buffer uniform_buffer;

    ui_renderer(vk::PhysicalDevice physical_device, vk::Device device);
    void update(vk::Device device) const;
    void draw(vk::CommandBuffer command_buffer) const;
    void destroy(vk::Device device) const;
};
