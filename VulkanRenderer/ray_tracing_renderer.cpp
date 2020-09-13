#include "stdafx.h"
#include "ray_tracing_renderer.h"
#include "dimensions.h"

void ray_tracing_renderer::initialize_ray_tracing_descriptor_set(vk::Device device, const ray_tracing_model* model)
{
    std::array buffer_infos = {
        vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf.get())
        .setRange(uniform_buffer.size)
    };

    auto uniform_buffer_descriptor = vk::WriteDescriptorSet()
                                     .setDstBinding(0)
                                     .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                     .setDstSet(ray_tracing_descriptor_set.get())
                                     .setBufferInfo(buffer_infos);

    std::array tlas{model->tlas->ac.get()};
    vk::StructureChain<vk::WriteDescriptorSet, vk::WriteDescriptorSetAccelerationStructureKHR> tlas_descriptor = {
        vk::WriteDescriptorSet()
        .setDstBinding(1)
        .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
        .setDstSet(ray_tracing_descriptor_set.get())
        .setDescriptorCount(1),

        vk::WriteDescriptorSetAccelerationStructureKHR()
        .setAccelerationStructures(tlas)
    };

    std::array images{
        vk::DescriptorImageInfo()
        .setImageView(image.image_view.get())
        .setImageLayout(vk::ImageLayout::eGeneral)
    };

    auto image_descriptor = vk::WriteDescriptorSet()
                            .setDstBinding(2)
                            .setDescriptorType(vk::DescriptorType::eStorageImage)
                            .setDstSet(ray_tracing_descriptor_set.get())
                            .setImageInfo(images);

    device.updateDescriptorSets({
                                    uniform_buffer_descriptor,
                                    tlas_descriptor.get<vk::WriteDescriptorSet>(),
                                    image_descriptor,
                                }, {});
}

ray_tracing_renderer::ray_tracing_renderer(vk::PhysicalDevice physical_device, vk::Device device,
                                           vk::DescriptorPool descriptor_pool, const pipeline* ray_tracing_pipeline,
                                           const buffer* shader_binding_table, const ray_tracing_model* model)
    : model(model),
      shader_binding_table(shader_binding_table),
      ray_tracing_pipeline(ray_tracing_pipeline),
      uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, HOST_VISIBLE_AND_COHERENT,
                     sizeof(model_uniform_data)),
      image(device, std::make_unique<image_with_memory>(physical_device, device, WIDTH, HEIGHT,
                                                        vk::Format::eR8G8B8A8Unorm,
                                                        vk::ImageUsageFlagBits::eStorage |
                                                        vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal,
                                                        vk::ImageLayout::eUndefined,
                                                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                        vk::ImageAspectFlagBits::eColor))
{
    std::array set_layouts{ray_tracing_pipeline->set_layout.get()};

    ray_tracing_descriptor_set = std::move(
        device.allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(descriptor_pool)
            .setSetLayouts(set_layouts)
        )[0]
    );

    initialize_ray_tracing_descriptor_set(device, model);
}

void ray_tracing_renderer::update(vk::Device device, model_uniform_data model_uniform_data) const
{
    uniform_buffer.update(device, &model_uniform_data);
}

void ray_tracing_renderer::draw_outside_renderpass(vk::CommandBuffer command_buffer) const
{
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::DependencyFlagBits(),
        {},
        {},
        {
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setImage(image.iwm->image.get())
            .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setSubresourceRange(image.iwm->sub_resource_range)
        }
    );

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, ray_tracing_pipeline->layout.get(), 0,
                                      ray_tracing_descriptor_set.get(), {});
    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, ray_tracing_pipeline->pl.get());

    auto entry_size = shader_binding_table->size / GROUP_COUNT;
    command_buffer.traceRaysNV(
        shader_binding_table->buf.get(),
        RAYGEN_SHADER_INDEX * entry_size,
        shader_binding_table->buf.get(),
        MISS_SHADER_INDEX * entry_size,
        0,
        shader_binding_table->buf.get(),
        CLOSEST_HIT_SHADER_INDEX * entry_size,
        0,
        nullptr,
        0,
        0,
        WIDTH,
        HEIGHT,
        1
    );

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits(),
        {},
        {},
        {
            vk::ImageMemoryBarrier()
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImage(image.iwm->image.get())
            .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setSubresourceRange(image.iwm->sub_resource_range)
        }
    );
}

void ray_tracing_renderer::draw(vk::CommandBuffer command_buffer) const
{
    //TODO render image
}
