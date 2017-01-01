#include "stdafx.h"
#include "buffer.h"

buffer::buffer(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags usage_flags, vk::DeviceSize allocation_size)
{
    this->allocation_size = allocation_size;

    buf = device.createBuffer(
        vk::BufferCreateInfo()
        .setSize(allocation_size)
        .setUsage(usage_flags)
    );

    auto desired_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    auto reqs = device.getBufferMemoryRequirements(buf);
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (auto i = uint32_t(0); i < props.memoryTypeCount; i++)
    {
        if ((reqs.memoryTypeBits & 1u << i) == 1u << i && (props.memoryTypes[i].propertyFlags & desired_flags) == desired_flags)
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
    memcpy(ptr, data, allocation_size);
    device.unmapMemory(memory);
}

void buffer::destroy(vk::Device device) const
{
    device.destroyBuffer(buf);
    device.freeMemory(memory);
}
