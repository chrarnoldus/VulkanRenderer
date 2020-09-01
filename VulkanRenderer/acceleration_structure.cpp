#include "stdafx.h"
#include "acceleration_structure.h"

#include "buffer.h"

acceleration_structure::acceleration_structure(vk::PhysicalDevice physical_device,
                                               vk::Device device, vk::CommandPool command_pool, vk::Queue queue,
                                               vk::AccelerationStructureInfoNV info)
{
    ac = device.createAccelerationStructureNVUnique(vk::AccelerationStructureCreateInfoNV().setInfo(info));

    auto memory_reqs = device.getAccelerationStructureMemoryRequirementsNV(
        vk::AccelerationStructureMemoryRequirementsInfoNV()
        .setAccelerationStructure(ac.get())
        .setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject)
    ).memoryRequirements;

    memory = device.allocateMemoryUnique(
        vk::MemoryAllocateInfo()
        .setAllocationSize(memory_reqs.size)
        .setMemoryTypeIndex(get_memory_index(physical_device, vk::MemoryPropertyFlagBits::eDeviceLocal, memory_reqs))
    );

    device.bindAccelerationStructureMemoryNV({
        vk::BindAccelerationStructureMemoryInfoKHR()
        .setAccelerationStructure(ac.get())
        .setMemory(memory.get())
    });

    auto scratch_requirements = device.getAccelerationStructureMemoryRequirementsNV(
        vk::AccelerationStructureMemoryRequirementsInfoNV()
        .setAccelerationStructure(ac.get())
        .setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch)
    ).memoryRequirements;

    buffer scratch(physical_device, device, vk::BufferUsageFlagBits::eRayTracingKHR,
                   vk::MemoryPropertyFlagBits::eDeviceLocal, scratch_requirements.size);

    auto command_buffer = std::move(device.allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0]);

    command_buffer->begin(vk::CommandBufferBeginInfo());
    command_buffer->buildAccelerationStructureNV(info, nullptr, 0, false, ac.get(), nullptr, scratch.buf.get(), 0);
    command_buffer->end();

    std::array command_buffers{command_buffer.get()};
    queue.submit({
                     vk::SubmitInfo().setCommandBuffers(command_buffers)
                 }, nullptr);
    queue.waitIdle();
}
