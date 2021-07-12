#include "stdafx.h"
#include "image_with_view.h"

image_with_memory::image_with_memory(
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
    : width(width), height(height), format(format)
{
    image = device.createImageUnique(
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

    const auto reqs = device.getImageMemoryRequirements(image.get());
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
    {
        if ((reqs.memoryTypeBits & 1u << i) == 1u << i && (props.memoryTypes[i].propertyFlags & memory_flags) ==
            memory_flags)
        {
            memory_type_index = i;
            break;
        }
    }
    assert(memory_type_index != UINT32_MAX);

    memory = device.allocateMemoryUnique(
        vk::MemoryAllocateInfo()
        .setMemoryTypeIndex(memory_type_index)
        .setAllocationSize(reqs.size)
    );

    device.bindImageMemory(image.get(), memory.get(), 0);

    sub_resource_range = vk::ImageSubresourceRange()
        .setAspectMask(aspect_flags)
        .setLevelCount(1)
        .setLayerCount(1);
}

std::unique_ptr<image_with_memory> image_with_memory::copy_from_host_to_device_for_shader_read(
    vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue) const
{
    auto result = std::make_unique<image_with_memory>(
        physical_device, device, width, height, format,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageTiling::eOptimal, vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
        );

    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo());
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eHost,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags(),
        {},
        {},
        {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eHostWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::ePreinitialized,
                vk::ImageLayout::eTransferSrcOptimal,
                0,
                0,
                image.get(),
                sub_resource_range
            ),
            vk::ImageMemoryBarrier(
                vk::AccessFlags(),
                vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                0,
                0,
                result->image.get(),
                result->sub_resource_range
            ),
        }
        );
    command_buffer.copyImage(
        image.get(), vk::ImageLayout::eTransferSrcOptimal, result->image.get(), vk::ImageLayout::eTransferDstOptimal, {
            vk::ImageCopy()
            .setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
            .setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
            .setExtent(vk::Extent3D(width, height, 1))
        });
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags(),
        {},
        {},
        {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                0,
                0,
                result->image.get(),
                result->sub_resource_range
            )
        }
        );
    command_buffer.end();

    queue.submit({ vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&command_buffer) }, nullptr);

    return result;
}

std::unique_ptr<image_with_memory> image_with_memory::copy_from_device_to_host(
    vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue) const
{
    auto result = std::make_unique<image_with_memory>(
        physical_device, device, width, height, format,
        vk::ImageUsageFlagBits::eTransferDst,
        vk::ImageTiling::eLinear, vk::ImageLayout::eUndefined,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vk::ImageAspectFlagBits::eColor
        );

    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo());
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags(),
        {},
        {},
        {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                0,
                0,
                image.get(),
                sub_resource_range
            )
        }
        );
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags(),
        {},
        {},
        {
            vk::ImageMemoryBarrier(
                vk::AccessFlags(),
                vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                0,
                0,
                result->image.get(),
                result->sub_resource_range
            )
        }
        );
    command_buffer.copyImage(
        image.get(), vk::ImageLayout::eTransferSrcOptimal, result->image.get(), vk::ImageLayout::eTransferDstOptimal, {
            vk::ImageCopy()
            .setSrcSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
            .setDstSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
            .setExtent(vk::Extent3D(width, height, 1))
        });
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eHost,
        vk::DependencyFlags(),
        {},
        {},
        {
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eHostRead,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eGeneral,
                0,
                0,
                result->image.get(),
                result->sub_resource_range
            )
        }
        );
    command_buffer.end();

    queue.submit({ vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&command_buffer) }, nullptr);

    return result;
}

std::unique_ptr<image_with_memory> load_r8g8b8a8_unorm_texture(vk::PhysicalDevice physical_device, vk::Device device,
    uint32_t width, uint32_t height, const void* data)
{
    auto image = std::make_unique<image_with_memory>(
        physical_device,
        device,
        width,
        height,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
        vk::ImageTiling::eLinear,
        vk::ImageLayout::ePreinitialized,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vk::ImageAspectFlagBits::eColor
        );

    const auto size = 4 * width * height;
    auto* ptr = device.mapMemory(image->memory.get(), 0, size);
    memcpy(ptr, data, size);
    device.unmapMemory(image->memory.get());
    return image;
}

image_with_view::image_with_view(vk::Device device, std::unique_ptr<image_with_memory> iwm)
    : iwm(std::move(iwm))
{
    image_view = device.createImageViewUnique(
        vk::ImageViewCreateInfo()
        .setImage(this->iwm->image.get())
        .setFormat(this->iwm->format)
        .setViewType(vk::ImageViewType::e2D)
        .setSubresourceRange(this->iwm->sub_resource_range)
    );
}
