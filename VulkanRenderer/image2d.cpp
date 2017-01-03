#include "stdafx.h"
#include "image2d.h"

image2d::image2d(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageUsageFlags usage_flags,
    vk::ImageTiling image_tiling,
    vk::ImageLayout initial_layout,
    vk::MemoryPropertyFlags memory_flags,
    vk::ImageAspectFlags aspect_flags
)
{
    image = device.createImage(
        vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setExtent(vk::Extent3D(width, height, 1))
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(image_tiling)
        .setUsage(usage_flags)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(initial_layout)
    );

    auto reqs = device.getImageMemoryRequirements(image);
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
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
        .setMemoryTypeIndex(memory_type_index)
        .setAllocationSize(reqs.size)
    );

    device.bindImageMemory(image, memory, 0);

    image_view = device.createImageView(
        vk::ImageViewCreateInfo()
        .setImage(image)
        .setFormat(format)
        .setViewType(vk::ImageViewType::e2D)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
            .setAspectMask(aspect_flags)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setLayerCount(1)
        )
    );
}

void image2d::destroy(vk::Device device) const
{
    device.destroyImageView(image_view);
    device.destroyImage(image);
    device.freeMemory(memory);
}

image2d load_r8g8b8a8_unorm_texture(vk::PhysicalDevice physical_device, vk::Device device, uint32_t width, uint32_t height, const void* data)
{
    image2d image(
        physical_device,
        device,
        width,
        height,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled,
        vk::ImageTiling::eLinear,
        vk::ImageLayout::ePreinitialized,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vk::ImageAspectFlagBits::eColor
    );

    auto size = 4 * width * height;
    auto ptr = device.mapMemory(image.memory, 0, size);
    memcpy(ptr, data, size);
    device.unmapMemory(image.memory);
    return image;
}
