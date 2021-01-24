#include "stdafx.h"
#include "acceleration_structure.h"

acceleration_structure::acceleration_structure(vk::PhysicalDevice physical_device,
                                               vk::Device device, vk::CommandPool command_pool, vk::Queue queue,
                                               const vk::AccelerationStructureGeometryKHR& geometry,
                                               vk::AccelerationStructureTypeKHR type,
                                               uint32_t max_primitives,
                                               std::unique_ptr<buffer> instance_data
): instance_data(std::move(instance_data))
{
    std::array geometries{ geometry };
    const auto build_sizes = device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(type)
            .setGeometries(geometries),
        {max_primitives}
    );

    ac_buffer = std::make_unique<buffer>(physical_device, device, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR, vk::MemoryPropertyFlagBits::eDeviceLocal, build_sizes.accelerationStructureSize);

    ac = device.createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR()
        .setType(type)
        .setBuffer(ac_buffer->buf.get())
        .setSize(ac_buffer->size)
    )
    ;

    buffer scratch(physical_device, device, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR|vk::BufferUsageFlagBits::eShaderDeviceAddress,
                   vk::MemoryPropertyFlagBits::eDeviceLocal, build_sizes.buildScratchSize);

    auto command_buffer = std::move(device.allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0]);

    const auto build_range_info = vk::AccelerationStructureBuildRangeInfoKHR()
        .setPrimitiveCount(max_primitives);

    command_buffer->begin(vk::CommandBufferBeginInfo());
    command_buffer->buildAccelerationStructuresKHR({
        vk::AccelerationStructureBuildGeometryInfoKHR()
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setDstAccelerationStructure(ac.get())
        .setType(type)
        .setScratchData(scratch.address)
        .setGeometries(geometries)
    }, {
        &build_range_info
    });
    command_buffer->end();

    std::array command_buffers{command_buffer.get()};
    queue.submit({
                     vk::SubmitInfo().setCommandBuffers(command_buffers)
                 }, nullptr);
    queue.waitIdle();
}
