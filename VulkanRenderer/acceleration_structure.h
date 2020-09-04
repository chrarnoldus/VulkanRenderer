#pragma once

struct acceleration_structure
{
    vk::UniqueDeviceMemory memory;
    vk::UniqueAccelerationStructureKHR ac;

    acceleration_structure(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool,
                           vk::Queue queue, vk::AccelerationStructureInfoNV info, vk::Buffer instanceData);
};
