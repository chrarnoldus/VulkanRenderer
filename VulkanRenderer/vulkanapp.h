#pragma once
#include <vulkan/vulkan.hpp>
#include "buffer.h"
#include "pipeline.h"

struct vulkanapp
{
    vk::Queue queue;
    buffer mesh;
    vk::RenderPass render_pass;
    pipeline pl;
    vk::DescriptorPool descriptor_pool;

    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface);
    void destroy(vk::Device device) const;
};

