#include "stdafx.h"
#include "model_renderer.h"


model_renderer::model_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool, pipeline model_pipeline, const model* mdl)
    : mdl(mdl)
    , model_pipeline(model_pipeline)
    , uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, HOST_VISIBLE_AND_COHERENT, sizeof(model_uniform_data))
{
    auto set_layouts = { model_pipeline.set_layout };
    descriptor_set = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(uint32_t(set_layouts.size()))
        .setPSetLayouts(set_layouts.begin())
    )[0];

    auto model_ub_info = vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf.get())
        .setRange(uniform_buffer.size);

    auto model_ub_write_description = vk::WriteDescriptorSet()
        .setDstBinding(0)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setDstSet(descriptor_set)
        .setPBufferInfo(&model_ub_info);

    device.updateDescriptorSets({ model_ub_write_description }, {});
}

void model_renderer::update(vk::Device device, model_uniform_data model_uniform_data) const
{
    uniform_buffer.update(device, &model_uniform_data);
}

void model_renderer::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, model_pipeline.layout, 0, descriptor_set, {});
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, model_pipeline.pl);
    mdl->draw(command_buffer);
}
