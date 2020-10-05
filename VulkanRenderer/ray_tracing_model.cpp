#include "stdafx.h"
#include "ray_tracing_model.h"

#include "data_types.h"

ray_tracing_model::ray_tracing_model(vk::PhysicalDevice physical_device, vk::Device device,
                                     vk::CommandPool command_pool, vk::Queue queue, const model* mdl)
    : mdl(mdl)
{
    std::array blas_geometries = {
        vk::GeometryNV()
        .setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setGeometry(
            vk::GeometryDataNV()
            .setTriangles(
                vk::GeometryTrianglesNV()
                .setIndexCount(mdl->index_count)
                .setIndexData(mdl->index_buffer->buf.get()) // TODO buffer usage flags?
                .setVertexData(mdl->vertex_buffer->buf.get())
                .setIndexType(vk::IndexType::eUint32)
                .setVertexCount(mdl->vertex_count)
                .setVertexFormat(vk::Format::eR16G16B16Snorm)
                .setVertexStride(sizeof(vertex))
                .setVertexOffset(offsetof(vertex, position))
            )
        )
    };

    auto blas_info = vk::AccelerationStructureInfoNV()
                     .setFlags(
                         vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction |
                         vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                     .setGeometries(blas_geometries)
                     .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

    blas = std::make_unique<acceleration_structure>(physical_device, device, command_pool, queue, blas_info, nullptr);

    auto tlas_info = vk::AccelerationStructureInfoNV()
                     .setFlags(
                         vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction |
                         vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                     .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                     .setInstanceCount(1);

    buffer instance_data(physical_device, device, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         sizeof(VkAccelerationStructureInstanceKHR));

    auto blas_reference = device.getAccelerationStructureHandleNV<uint64_t>(blas->ac.get());

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
        physical_device, device, vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eTransferDst,
        command_pool, queue); //TODO pipeline barrier

    tlas = std::make_unique<acceleration_structure>(physical_device, device, command_pool, queue, tlas_info,
                                                    instance_data_device_local->buf.get());
}
