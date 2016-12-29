#include "stdafx.h"
#include "frame.h"
#include "dimensions.h"

static vk::CommandBuffer create_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::RenderPass render_pass, pipeline pipeline, vk::Framebuffer framebuffer, buffer buf)
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
        .setColor(vk::ClearColorValue().setFloat32({ 0.f, 1.f, 1.f, 1.f }));

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

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, pipeline.descriptor_sets, {});

    command_buffer.bindVertexBuffers(0, { buf.buf }, { 0 });

    command_buffer.draw(6, 1, 0, 0);

    command_buffer.endRenderPass();

    command_buffer.end();

    return command_buffer;
}

frame::frame(vk::Device device, vk::CommandPool command_pool, vk::Image image, vk::RenderPass render_pass, pipeline pipeline, buffer vertex_buffer)
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

    command_buffer = create_command_buffer(device, command_pool, render_pass, pipeline, framebuffer, vertex_buffer);

    submitted_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
}

void frame::destroy(vk::Device device) const
{
    device.destroyFramebuffer(framebuffer);
    device.destroyImageView(image_view);
    device.destroyImage(image);
    device.destroySemaphore(submitted_semaphore);
}
