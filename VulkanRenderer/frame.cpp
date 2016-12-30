#include "stdafx.h"
#include "frame.h"
#include "dimensions.h"

static vk::CommandBuffer create_command_buffer(vk::Device device, vk::CommandPool command_pool, const std::vector<vk::DescriptorSet>& descriptor_sets, vk::RenderPass render_pass, pipeline pipeline, vk::Framebuffer framebuffer, buffer buf)
{
    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));

    auto clear_value = vk::ClearValue()
        .setColor(vk::ClearColorValue().setFloat32({0.f, 1.f, 1.f, 1.f}));

    vk::Rect2D render_area;
    render_area.extent.width = WIDTH;
    render_area.extent.height = HEIGHT;

    command_buffer.beginRenderPass(
        vk::RenderPassBeginInfo()
        .setRenderPass(render_pass)
        .setClearValueCount(1)
        .setPClearValues(&clear_value)
        .setRenderArea(render_area)
        .setFramebuffer(framebuffer),
        vk::SubpassContents::eInline
    );

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pl);

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, descriptor_sets, {});

    command_buffer.bindVertexBuffers(0, {buf.buf}, {0});

    command_buffer.draw(6, 1, 0, 0);

    command_buffer.endRenderPass();

    command_buffer.end();

    return command_buffer;
}

frame::frame(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    vk::CommandPool command_pool,
    vk::DescriptorPool descriptor_pool,
    vk::Image image,
    vk::RenderPass render_pass,
    pipeline pipeline,
    buffer vertex_buffer)
    : uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(uniform_data))
{
    this->image = image;

    image_view = device.createImageView(
        vk::ImageViewCreateInfo()
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setViewType(vk::ImageViewType::e2D)
        .setImage(image)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setLayerCount(1)
        )
    );

    framebuffer = device.createFramebuffer(
        vk::FramebufferCreateInfo()
        .setRenderPass(render_pass)
        .setAttachmentCount(1)
        .setPAttachments(&image_view)
        .setWidth(WIDTH)
        .setHeight(HEIGHT)
        .setLayers(1)
    );

    rendered_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());

    rendered_fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

    auto descriptor_sets = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&pipeline.set_layout)
    );

    auto buffer_info = vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf)
        .setRange(uniform_buffer.allocation_size);

    auto write_descriptor_set = vk::WriteDescriptorSet()
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setDstSet(descriptor_sets[0])
        .setPBufferInfo(&buffer_info);

    device.updateDescriptorSets({ write_descriptor_set }, {});

    command_buffer = create_command_buffer(device, command_pool, descriptor_sets, render_pass, pipeline, framebuffer, vertex_buffer);
}

void frame::destroy(vk::Device device) const
{
    uniform_buffer.destroy(device);
    device.destroyFramebuffer(framebuffer);
    device.destroyImageView(image_view);
    device.destroyImage(image);
    device.destroySemaphore(rendered_semaphore);
    device.destroyFence(rendered_fence);
}
