#include "stdafx.h"
#include "frame.h"
#include "data_types.h"
#include "dimensions.h"

static void record_command_buffer(
    vk::CommandBuffer command_buffer,
    vk::RenderPass render_pass,
    const std::vector<std::unique_ptr<renderer>>& renderers,
    vk::Framebuffer framebuffer
)
{
    command_buffer.begin(vk::CommandBufferBeginInfo());

    for (const auto& renderer : renderers)
    {
        renderer->draw_outside_renderpass(command_buffer);
    }

    std::array clear_values{
        vk::ClearValue().setColor(vk::ClearColorValue().setFloat32({0.f, 1.f, 1.f, 1.f})),
        vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue().setDepth(1.f))
    };

    command_buffer.beginRenderPass(
        vk::RenderPassBeginInfo()
        .setRenderPass(render_pass)
        .setClearValues(clear_values)
        .setRenderArea(vk::Rect2D().setExtent(vk::Extent2D(WIDTH, HEIGHT)))
        .setFramebuffer(framebuffer),
        vk::SubpassContents::eInline
    );

    for (const auto& renderer : renderers)
    {
        renderer->draw(command_buffer);
    }

    command_buffer.endRenderPass();
    command_buffer.end();
}

frame::frame(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    vk::CommandPool command_pool,
    vk::DescriptorPool descriptor_pool,
    vk::Image image,
    vk::Format format,
    vk::RenderPass render_pass,
    std::vector<std::unique_ptr<renderer>> renderers)
    : device(device), dsb(device, std::make_unique<image_with_memory>(physical_device, device, WIDTH, HEIGHT,
                                                                      vk::Format::eD24UnormS8Uint,
                                                                      vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                                      vk::ImageTiling::eOptimal,
                                                                      vk::ImageLayout::eUndefined,
                                                                      vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                                      vk::ImageAspectFlagBits::eDepth |
                                                                      vk::ImageAspectFlagBits::eStencil))
      , renderers(std::move(renderers))
{
    this->image = image;

    image_view = device.createImageViewUnique(
        vk::ImageViewCreateInfo()
        .setFormat(format)
        .setViewType(vk::ImageViewType::e2D)
        .setImage(image)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(1)
            .setLayerCount(1)
        )
    );

    std::array attachments{image_view.get(), dsb.image_view.get()};
    framebuffer = device.createFramebufferUnique(
        vk::FramebufferCreateInfo()
        .setRenderPass(render_pass)
        .setAttachments(attachments)
        .setWidth(WIDTH)
        .setHeight(HEIGHT)
        .setLayers(1)
    );

    rendered_fence = device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    record_command_buffer(command_buffer, render_pass, this->renderers, framebuffer.get());
}

void frame::update(model_uniform_data model_uniform_data) const
{
    for (const auto& renderer : renderers)
    {
        renderer->update(device, model_uniform_data);
    }
}
