#include "stdafx.h"
#include "model_renderer.h"


model_renderer::model_renderer(vk::PhysicalDevice physical_device, vk::Device device,
                               vk::DescriptorPool descriptor_pool, vk::Extent2D framebuffer_size,
                               const pipeline* model_pipeline, const model* mdl)
    : mdl(mdl)
      , model_pipeline(model_pipeline)
      , uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, HOST_VISIBLE_AND_COHERENT,
                       sizeof(model_uniform_data))
      , framebuffer_size(framebuffer_size)
{
    std::array set_layouts{model_pipeline->set_layout.get()};
    descriptor_set = std::move(device.allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setSetLayouts(set_layouts)
    )[0]);

    auto model_ub_info = vk::DescriptorBufferInfo()
                         .setBuffer(uniform_buffer.buf.get())
                         .setRange(uniform_buffer.size);

    const auto model_ub_write_description = vk::WriteDescriptorSet()
                                            .setDstBinding(0)
                                            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                            .setDescriptorCount(1)
                                            .setDstSet(descriptor_set.get())
                                            .setPBufferInfo(&model_ub_info);

    device.updateDescriptorSets({model_ub_write_description}, {});
}

void model_renderer::update(vk::Device device, model_uniform_data model_uniform_data) const
{
    uniform_buffer.update(device, &model_uniform_data);
}

void model_renderer::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, model_pipeline->layout.get(), 0,
                                      descriptor_set.get(),
                                      {});

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, model_pipeline->pl.get());

    command_buffer.setViewport(0, {
                                   vk::Viewport().setWidth(static_cast<float>(framebuffer_size.width))
                                                 .setY(static_cast<float>(framebuffer_size.height))
                                                 .setHeight(-static_cast<float>(framebuffer_size.height))
                                                 .setMaxDepth(1.0)
                               });

    command_buffer.setScissor(0, {vk::Rect2D().setExtent(framebuffer_size)});

    mdl->draw(command_buffer);
}
