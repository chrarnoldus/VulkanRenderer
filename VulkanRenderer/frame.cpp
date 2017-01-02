#include "stdafx.h"
#include "frame.h"
#include "data_types.h"
#include "dimensions.h"

static vk::CommandBuffer create_command_buffer(vk::Device device, vk::CommandPool command_pool, const std::vector<vk::DescriptorSet>& descriptor_sets, vk::RenderPass render_pass, pipeline pipeline, vk::Framebuffer framebuffer, model model)
{
    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));

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

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pl);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, descriptor_sets, {});
    model.draw(command_buffer);

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
    model model)
    : uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(uniform_data))
      , dsb(physical_device, device, WIDTH, HEIGHT)
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
    vk::ImageView attachments[attachment_count] = {image_view, dsb.image_view};
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

    auto descriptor_sets = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&pipeline.set_layout)
    );

    auto buffer_info = vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf)
        .setRange(uniform_buffer.size);

    auto write_descriptor_set = vk::WriteDescriptorSet()
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setDstSet(descriptor_sets[0])
        .setPBufferInfo(&buffer_info);

    device.updateDescriptorSets({write_descriptor_set}, {});

    command_buffer = create_command_buffer(device, command_pool, descriptor_sets, render_pass, pipeline, framebuffer, model);
}

void frame::destroy(vk::Device device) const
{
    dsb.destroy(device);
    uniform_buffer.destroy(device);
    device.destroyFramebuffer(framebuffer);
    device.destroyImageView(image_view);
    device.destroyFence(rendered_fence);
}
