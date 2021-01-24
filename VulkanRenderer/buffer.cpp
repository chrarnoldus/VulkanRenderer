#include "stdafx.h"
#include "buffer.h"

uint32_t get_memory_index(vk::PhysicalDevice physical_device, vk::MemoryPropertyFlags memory_flags,
    vk::MemoryRequirements reqs)
{
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (auto i = static_cast<uint32_t>(0); i < props.memoryTypeCount; i++)
    {
        if ((reqs.memoryTypeBits & 1u << i) == 1u << i && (props.memoryTypes[i].propertyFlags & memory_flags) ==
            memory_flags)
        {
            memory_type_index = i;
            break;
        }
    }
    assert(memory_type_index != UINT32_MAX);

    return memory_type_index;
}

buffer::buffer(vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags usage_flags,
    vk::MemoryPropertyFlags memory_flags, vk::DeviceSize size)
{
    this->size = size;

    buf = device.createBufferUnique(
        vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage_flags)
    );

    const auto reqs = device.getBufferMemoryRequirements(buf.get());
    const uint32_t memory_type_index = get_memory_index(physical_device, memory_flags, reqs);

    auto shader_device_address = (usage_flags & vk::BufferUsageFlagBits::eShaderDeviceAddress) == vk::BufferUsageFlagBits::eShaderDeviceAddress;

    const auto info = vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryAllocateFlagsInfo>{
        vk::MemoryAllocateInfo()
        .setAllocationSize(reqs.size)
        .setMemoryTypeIndex(memory_type_index),
        vk::MemoryAllocateFlagsInfo()
        .setFlags(shader_device_address ? vk::MemoryAllocateFlagBits::eDeviceAddress : vk::MemoryAllocateFlagBits())
    };

    memory = device.allocateMemoryUnique(info.get());

    device.bindBufferMemory(buf.get(), memory.get(), 0);

    if (shader_device_address) {
        address = device.getBufferAddress(buf.get());
    }
    else {
        address = 0;
    }
}

void buffer::update(vk::Device device, void* data) const
{
    auto* ptr = device.mapMemory(memory.get(), 0, VK_WHOLE_SIZE);
    memcpy(ptr, data, size);
    device.unmapMemory(memory.get());
}

std::unique_ptr<buffer> buffer::copy_from_host_to_device_for_vertex_input(
    vk::PhysicalDevice physical_device, vk::Device device, vk::BufferUsageFlags new_usage_flags,
    vk::CommandPool command_pool, vk::Queue queue) const
{
    auto result = std::make_unique<buffer>(physical_device, device, new_usage_flags,
        vk::MemoryPropertyFlagBits::eDeviceLocal, size);

    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo());
    command_buffer.copyBuffer(buf.get(), result->buf.get(), { vk::BufferCopy(0, 0, size) });

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::DependencyFlags(),
        {},
        {
            vk::BufferMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead)
            .setBuffer(result->buf.get())
            .setSize(result->size)
        },
        {}
    );

    command_buffer.end();

    queue.submit({ vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&command_buffer) }, nullptr);

    return result;
}
