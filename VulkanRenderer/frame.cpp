#include "stdafx.h"
#include "frame.h"
#include "data_types.h"
#include "dimensions.h"

static void record_command_buffer(
    vk::CommandBuffer command_buffer,
    vk::DescriptorSet descriptor_set,
    vk::RenderPass render_pass,
    pipeline model_pipeline,
    model model,
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

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, model_pipeline.layout, 0, descriptor_set, {});
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, model_pipeline.pl);
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
    : uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, HOST_VISIBLE_AND_COHERENT, sizeof(model_uniform_data))
    , dsb(physical_device, device, WIDTH, HEIGHT, vk::Format::eD24UnormS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
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

    auto set_layouts = { model_pipeline.set_layout , ui_pipeline.set_layout };
    auto descriptor_set = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(uint32_t(set_layouts.size()))
        .setPSetLayouts(set_layouts.begin())
    )[0];

    auto model_ub_info = vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf)
        .setRange(uniform_buffer.size);

    auto model_ub_write_description = vk::WriteDescriptorSet()
        .setDstBinding(0)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setDstSet(descriptor_set)
        .setPBufferInfo(&model_ub_info);

    device.updateDescriptorSets({ model_ub_write_description }, {});

    command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    record_command_buffer(command_buffer, descriptor_set, render_pass, model_pipeline, model, ui, framebuffer);
}

void frame::destroy(vk::Device device) const
{
    ui.destroy(device);
    dsb.destroy(device);
    uniform_buffer.destroy(device);
    device.destroyFramebuffer(framebuffer);
    device.destroyImageView(image_view);
    device.destroyFence(rendered_fence);
}
