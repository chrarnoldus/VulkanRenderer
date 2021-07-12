#pragma once
#include "buffer.h"

struct acceleration_structure
{
    std::unique_ptr<buffer> ac_buffer;
    std::unique_ptr<buffer> instance_data;
    vk::UniqueAccelerationStructureKHR ac;

    acceleration_structure(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool,
        vk::Queue queue, const vk::AccelerationStructureGeometryKHR& geometry, vk::AccelerationStructureTypeKHR type, uint32_t max_primitives, std::unique_ptr<buffer> instance_data);
};
