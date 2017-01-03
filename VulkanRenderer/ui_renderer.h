#pragma once
#include <vulkan/vulkan.hpp>

#include "buffer.h"
#include "image2d.h"

class ui_renderer
{
    buffer vertex_buffer;
    buffer index_buffer;
    buffer indirect_buffer;
    image2d font_image;

public:
    ui_renderer(vk::PhysicalDevice physical_device, vk::Device device);
    void update(vk::Device device) const;
    void draw(vk::CommandBuffer command_buffer) const;
    void destroy(vk::Device device) const;
};
