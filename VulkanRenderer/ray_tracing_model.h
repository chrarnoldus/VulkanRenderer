#pragma once
#include "acceleration_structure.h"
#include "model.h"

struct ray_tracing_model
{
    model model;
    std::unique_ptr<acceleration_structure> blas;
    std::unique_ptr<acceleration_structure> tlas;
    ray_tracing_model(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool,
                      vk::Queue queue, const std::string& path);
};
