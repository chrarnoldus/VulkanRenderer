#include "stdafx.h"
#include "depth_stencil_buffer.h"

depth_stencil_buffer::depth_stencil_buffer(vk::PhysicalDevice physical_device, vk::Device device, uint32_t width, uint32_t height)
{
    image = device.createImage(
        vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eD24UnormS8Uint)
        .setExtent(vk::Extent3D(width, height, 1))
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
        .setSharingMode(vk::SharingMode::eExclusive)
    );

    auto reqs = device.getImageMemoryRequirements(image);
    auto desired_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
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
        .setMemoryTypeIndex(memory_type_index)
        .setAllocationSize(reqs.size)
    );

    device.bindImageMemory(image, memory, 0);

    image_view = device.createImageView(
        vk::ImageViewCreateInfo()
        .setImage(image)
        .setFormat(vk::Format::eD24UnormS8Uint)
        .setViewType(vk::ImageViewType::e2D)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setLayerCount(1)
        )
    );
}

void depth_stencil_buffer::destroy(vk::Device device) const
{
    device.destroyImageView(image_view);
    device.destroyImage(image);
    device.freeMemory(memory);
}
