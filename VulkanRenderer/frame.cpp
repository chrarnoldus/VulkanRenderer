#include "stdafx.h"
#include "frame.h"
#include "data_types.h"
#include "dimensions.h"

static void record_command_buffer(
    vk::CommandBuffer command_buffer,
    vk::RenderPass render_pass,
    model_renderer model,
    ui_renderer ui,
    vk::Framebuffer framebuffer
)
{
    command_buffer.begin(vk::CommandBufferBeginInfo());

    const uint32_t clear_value_count = 2;
    vk::ClearValue clear_values[clear_value_count] = {
        vk::ClearValue().setColor(vk::ClearColorValue().setFloat32({0.f, 1.f, 1.f, 1.f})),
        vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue().setDepth(1.f))
    };

    command_buffer.beginRenderPass(
        vk::RenderPassBeginInfo()
        .setRenderPass(render_pass)
        .setClearValueCount(clear_value_count)
        .setPClearValues(clear_values)
        .setRenderArea(vk::Rect2D().setExtent(vk::Extent2D(WIDTH, HEIGHT)))
        .setFramebuffer(framebuffer),
        vk::SubpassContents::eInline
    );

    model.draw(command_buffer);
    ui.draw(command_buffer);

    command_buffer.endRenderPass();
    command_buffer.end();
}

frame::frame(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    vk::CommandPool command_pool,
    vk::DescriptorPool descriptor_pool,
    vk::Image image,
    vk::RenderPass render_pass,
    pipeline model_pipeline,
    pipeline ui_pipeline,
    model model,
    image2d font_image)
    : dsb(physical_device, device, WIDTH, HEIGHT, vk::Format::eD24UnormS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
    , mdl(physical_device, device, descriptor_pool, model_pipeline, model)
    , ui(physical_device, device, descriptor_pool, ui_pipeline, font_image)
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

    const uint32_t attachment_count = 2;
    vk::ImageView attachments[attachment_count] = { image_view, dsb.image_view };
    framebuffer = device.createFramebuffer(
        vk::FramebufferCreateInfo()
        .setRenderPass(render_pass)
        .setAttachmentCount(attachment_count)
        .setPAttachments(attachments)
        .setWidth(WIDTH)
        .setHeight(HEIGHT)
        .setLayers(1)
    );

    rendered_fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    record_command_buffer(command_buffer, render_pass, mdl, ui, framebuffer);
}

void frame::destroy(vk::Device device) const
{
    ui.destroy(device);
    mdl.destroy(device);
    dsb.destroy(device);
    device.destroyFramebuffer(framebuffer);
    device.destroyImageView(image_view);
    device.destroyFence(rendered_fence);
}
