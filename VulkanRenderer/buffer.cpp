#include "stdafx.h"
#include "buffer.h"

buffer::buffer(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags, vk::DeviceSize size)
{
    this->size = size;

    buf = device.createBuffer(
        vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage_flags)
    );

    auto reqs = device.getBufferMemoryRequirements(buf);
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (auto i = uint32_t(0); i < props.memoryTypeCount; i++)
    {
        if ((reqs.memoryTypeBits & 1u << i) == 1u << i && (props.memoryTypes[i].propertyFlags & memory_flags) == memory_flags)
        {
            memory_type_index = i;
            break;
        }
    }
    assert(memory_type_index != UINT32_MAX);

    memory = device.allocateMemory(
        vk::MemoryAllocateInfo()
        .setAllocationSize(reqs.size)
        .setMemoryTypeIndex(memory_type_index)
    );

    device.bindBufferMemory(buf, memory, 0);
}

void buffer::update(vk::Device device, void* data) const
{
    auto ptr = device.mapMemory(memory, 0, VK_WHOLE_SIZE);
    memcpy(ptr, data, size);
    device.unmapMemory(memory);
}

buffer buffer::copy_from_host_to_device_for_vertex_input(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags new_usage_flags, vk::CommandPool command_pool, vk::Queue queue) const
{
    buffer result(physical_device, device, new_usage_flags, vk::MemoryPropertyFlagBits::eDeviceLocal, size);

    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo());
    command_buffer.copyBuffer(buf, result.buf, { vk::BufferCopy(0, 0, size) });

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::DependencyFlags(),
        {},
        { vk::BufferMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead)
            .setBuffer(result.buf)
            .setSize(result.size) },
            {}
    );

    command_buffer.end();

    queue.submit({ vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&command_buffer) }, nullptr);

    return result;
}

void buffer::destroy(vk::Device device) const
{
    device.destroyBuffer(buf);
    device.freeMemory(memory);
}
