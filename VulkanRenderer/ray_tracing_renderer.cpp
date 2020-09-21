#include "stdafx.h"
#include "ray_tracing_renderer.h"
#include "dimensions.h"

void ray_tracing_renderer::initialize_ray_tracing_descriptor_set(vk::Device device)
{
    std::array uniform_buffer_infos{
        vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf.get())
        .setRange(uniform_buffer.size)
    };

    auto uniform_buffer_descriptor = vk::WriteDescriptorSet()
                                     .setDstBinding(0)
                                     .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                     .setDstSet(ray_tracing_descriptor_set.get())
                                     .setBufferInfo(uniform_buffer_infos);

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

    std::array vertex_buffer_infos{
        vk::DescriptorBufferInfo()
        .setBuffer(model->mdl->vertex_buffer->buf.get())
        .setRange(model->mdl->vertex_buffer->size)
    };

    auto vertex_buffer_descriptor = vk::WriteDescriptorSet()
                                    .setDstBinding(3)
                                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                    .setDstSet(ray_tracing_descriptor_set.get())
                                    .setBufferInfo(vertex_buffer_infos);


    std::array index_buffer_infos{
        vk::DescriptorBufferInfo()
        .setBuffer(model->mdl->index_buffer->buf.get())
        .setRange(model->mdl->index_buffer->size)
    };

    auto index_buffer_descriptor = vk::WriteDescriptorSet()
                                   .setDstBinding(4)
                                   .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                   .setDstSet(ray_tracing_descriptor_set.get())
                                   .setBufferInfo(index_buffer_infos);

    device.updateDescriptorSets({
                                    uniform_buffer_descriptor,
                                    tlas_descriptor.get<vk::WriteDescriptorSet>(),
                                    image_descriptor,
                                    vertex_buffer_descriptor,
                                    index_buffer_descriptor,
                                }, {});
}

ray_tracing_renderer::ray_tracing_renderer(vk::PhysicalDevice physical_device, vk::Device device,
                                           vk::DescriptorPool descriptor_pool, const pipeline* ray_tracing_pipeline,
                                           const pipeline* textured_quad_pipeline,
                                           const buffer* shader_binding_table, const ray_tracing_model* model)
    : model(model),
      shader_binding_table(shader_binding_table),
      ray_tracing_pipeline(ray_tracing_pipeline),
      textured_quad_pipeline(textured_quad_pipeline),
      uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, HOST_VISIBLE_AND_COHERENT,
                     sizeof(model_uniform_data)),
      textured_quad(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, HOST_VISIBLE_AND_COHERENT,
                    4 * sizeof(glm::vec2)),
      image(device, std::make_unique<image_with_memory>(physical_device, device, WIDTH, HEIGHT,
                                                        vk::Format::eR8G8B8A8Unorm,
                                                        vk::ImageUsageFlagBits::eStorage |
                                                        vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal,
                                                        vk::ImageLayout::eUndefined,
                                                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                        vk::ImageAspectFlagBits::eColor))
{
    std::array set_layouts{
        ray_tracing_pipeline->set_layout.get(),
        textured_quad_pipeline->set_layout.get(),
    };

    auto descriptor_sets = device.allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setSetLayouts(set_layouts)
    );

    ray_tracing_descriptor_set = std::move(descriptor_sets[0]);
    textured_quad_descriptor_set = std::move(descriptor_sets[1]);

    initialize_ray_tracing_descriptor_set(device);

    auto* ptr = static_cast<glm::vec2*>(device.mapMemory(textured_quad.memory.get(), 0, textured_quad.size));
    ptr[0] = glm::vec2(-1.f, -1.f);
    ptr[1] = glm::vec2(-1.f, 1.f);
    ptr[2] = glm::vec2(1.f, -1.f);
    ptr[3] = glm::vec2(1.f, 1.f);
    device.unmapMemory(textured_quad.memory.get());

    std::array images{
        vk::DescriptorImageInfo()
        .setImageView(image.image_view.get())
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
    };

    auto image_descriptor = vk::WriteDescriptorSet()
                            .setDstBinding(0)
                            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                            .setDstSet(textured_quad_descriptor_set.get())
                            .setImageInfo(images);

    device.updateDescriptorSets({image_descriptor}, {});
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
        image.iwm->width,
        image.iwm->height,
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
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, textured_quad_pipeline->layout.get(), 0,
                                      textured_quad_descriptor_set.get(), {});
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, textured_quad_pipeline->pl.get());

    command_buffer.bindVertexBuffers(0, {textured_quad.buf.get()}, {0});
    command_buffer.draw(4, 1, 0, 0);
}
