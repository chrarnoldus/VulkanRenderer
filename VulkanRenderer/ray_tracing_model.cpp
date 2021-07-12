#include "stdafx.h"
#include "ray_tracing_model.h"

#include "data_types.h"

ray_tracing_model::ray_tracing_model(vk::PhysicalDevice physical_device, vk::Device device,
    vk::CommandPool command_pool, vk::Queue queue, const model* mdl)
    : mdl(mdl)
{
    auto blas_geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setGeometry(
            vk::AccelerationStructureGeometryDataKHR()
            .setTriangles(
                vk::AccelerationStructureGeometryTrianglesDataKHR()
                .setIndexData(device.getBufferAddress(mdl->index_buffer->buf.get())) // TODO buffer usage flags?
                .setVertexData(device.getBufferAddress(mdl->vertex_buffer->buf.get()))
                .setIndexType(vk::IndexType::eUint32)
                .setMaxVertex(mdl->vertex_count - 1)
                .setVertexFormat(vk::Format::eR16G16B16Snorm)
                .setVertexStride(sizeof(vertex))
            )
        );

    blas = std::make_unique<acceleration_structure>(physical_device, device, command_pool, queue, blas_geometry, vk::AccelerationStructureTypeKHR::eBottomLevel, mdl->index_count / 3, nullptr);

    buffer instance_data(physical_device, device, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        sizeof(VkAccelerationStructureInstanceKHR));

    auto blas_reference = device.getAccelerationStructureAddressKHR(
        vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(blas->ac.get())
    );

    auto* ptr = static_cast<vk::AccelerationStructureInstanceKHR*>(device.mapMemory(
        instance_data.memory.get(), 0, instance_data.size));

    const std::array<std::array<float, 4>, 3> identity{
        std::array<float, 4>{1.f, 0.f, 0.f, 0.f},
        std::array<float, 4>{0.f, 1.f, 0.f, 0.f},
        std::array<float, 4>{0.f, 0.f, 1.f, 0.f},
    };
    *ptr = vk::AccelerationStructureInstanceKHR()
        .setTransform(vk::TransformMatrixKHR(identity))
        .setMask(UINT8_MAX)
        .setInstanceShaderBindingTableRecordOffset(0/*TODO*/)
        .setFlags(vk::GeometryInstanceFlagBitsKHR())
        .setAccelerationStructureReference(blas_reference);

    device.unmapMemory(instance_data.memory.get());

    auto instance_data_device_local = instance_data.copy_from_host_to_device_for_vertex_input(
        physical_device, device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        command_pool, queue); //TODO pipeline barrier

    auto tlas_geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry(vk::AccelerationStructureGeometryDataKHR().setInstances(
            vk::AccelerationStructureGeometryInstancesDataKHR()
            .setData(device.getBufferAddress(instance_data_device_local->buf.get()))
        ));

    tlas = std::make_unique<acceleration_structure>(physical_device, device, command_pool, queue, tlas_geometry, vk::AccelerationStructureTypeKHR::eTopLevel, 1, std::move(instance_data_device_local));
}
