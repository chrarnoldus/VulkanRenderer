#pragma once
#include <vulkan/vulkan.hpp>
#include "data_types.h"

class renderer {
public:
    virtual void update(vk::Device device, model_uniform_data model_uniform_data) const = 0;
    virtual void draw_outside_renderpass(vk::CommandBuffer command_buffer) const = 0;
    virtual void draw(vk::CommandBuffer command_buffer) const = 0;
};
