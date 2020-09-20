#include "stdafx.h"
#include "data_types.h"
#include "pipeline.h"
#include "dimensions.h"

static uint32_t model_vert_shader_spv[] = {
#include "model.vert.num"
};

static uint32_t model_frag_shader_spv[] = {
#include "model.frag.num"
};

static uint32_t model_rgen_shader_spv[]{
#include "model.rgen.num"
};

static uint32_t model_rchit_shader_spv[] = {
#include "model.rchit.num"
};

static uint32_t model_rmiss_shader_spv[] = {
#include "model.rmiss.num"
};

static uint32_t ui_vert_shader_spv[] = {
#include "ui.vert.num"
};

static uint32_t ui_frag_shader_spv[] = {
#include "ui.frag.num"
};

static uint32_t textured_quad_vert_spv[] = {
#include "textured_quad.vert.num"
};

static uint32_t textured_quad_frag_spv[] = {
#include "textured_quad.frag.num"
};

pipeline create_ui_pipeline(vk::Device device, vk::RenderPass render_pass)
{
    auto vert_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(ui_vert_shader_spv))
        .setPCode(ui_vert_shader_spv)
    );

    auto frag_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(ui_frag_shader_spv))
        .setPCode(ui_frag_shader_spv)
    );

    auto vert_stage = vk::PipelineShaderStageCreateInfo()
                      .setStage(vk::ShaderStageFlagBits::eVertex)
                      .setModule(vert_shader)
                      .setPName("main");

    auto frag_stage = vk::PipelineShaderStageCreateInfo()
                      .setModule(frag_shader)
                      .setStage(vk::ShaderStageFlagBits::eFragment)
                      .setPName("main");

    std::array stages{vert_stage, frag_stage};

    auto input_binding = vk::VertexInputBindingDescription()
        .setStride(sizeof(ImDrawVert));

    auto position_attribute = vk::VertexInputAttributeDescription()
                              .setFormat(vk::Format::eR32G32Sfloat)
                              .setLocation(0)
                              .setOffset(offsetof(ImDrawVert, pos));

    auto uv_attribute = vk::VertexInputAttributeDescription()
                        .setFormat(vk::Format::eR32G32Sfloat)
                        .setLocation(1)
                        .setOffset(offsetof(ImDrawVert, uv));

    auto color_attribute = vk::VertexInputAttributeDescription()
                           .setFormat(vk::Format::eR8G8B8A8Unorm)
                           .setLocation(2)
                           .setOffset(offsetof(ImDrawVert, col));

    std::array attributes{position_attribute, uv_attribute, color_attribute};

    auto input_state = vk::PipelineVertexInputStateCreateInfo()
                       .setVertexBindingDescriptionCount(1)
                       .setPVertexBindingDescriptions(&input_binding)
                       .setVertexAttributeDescriptions(attributes);

    auto assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport = vk::Viewport().setWidth(static_cast<float>(WIDTH)).setHeight(static_cast<float>(HEIGHT)).
                                   setMaxDepth(1.0);
    auto scissor = vk::Rect2D().setExtent(vk::Extent2D(WIDTH, HEIGHT));

    auto viewport_state = vk::PipelineViewportStateCreateInfo()
                          .setViewportCount(1)
                          .setPViewports(&viewport)
                          .setScissorCount(1)
                          .setPScissors(&scissor);

    auto rasterization_state = vk::PipelineRasterizationStateCreateInfo()
        .setLineWidth(1.f);

    auto multisample_state = vk::PipelineMultisampleStateCreateInfo()
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    auto attachment_state = vk::PipelineColorBlendAttachmentState()
                            .setColorWriteMask(
                                vk::ColorComponentFlagBits::eR |
                                vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB |
                                vk::ColorComponentFlagBits::eA
                            )
                            .setBlendEnable(true)
                            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                            .setSrcAlphaBlendFactor(vk::BlendFactor::eZero)
                            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                            .setColorBlendOp(vk::BlendOp::eAdd)
                            .setAlphaBlendOp(vk::BlendOp::eAdd);

    auto blend_state = vk::PipelineColorBlendStateCreateInfo()
                       .setAttachmentCount(1)
                       .setPAttachments(&attachment_state);

    auto depth_stencil_state = vk::PipelineDepthStencilStateCreateInfo();

    std::vector samplers{
        device.createSampler(
            vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        )
    };

    auto uniform_binding = vk::DescriptorSetLayoutBinding()
                           .setBinding(0)
                           .setDescriptorCount(1)
                           .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                           .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    auto sampler_binding = vk::DescriptorSetLayoutBinding()
                           .setBinding(1)
                           .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                           .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                           .setImmutableSamplers(samplers);

    std::array bindings = {uniform_binding, sampler_binding};

    auto set_layout = device.createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindings(bindings)
    );
    auto layout = device.createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout.get())
    );

    auto pl = device.createGraphicsPipelineUnique(
        nullptr,
        vk::GraphicsPipelineCreateInfo()
        .setStages(stages)
        .setPVertexInputState(&input_state)
        .setPInputAssemblyState(&assembly_state)
        .setPViewportState(&viewport_state)
        .setPRasterizationState(&rasterization_state)
        .setPMultisampleState(&multisample_state)
        .setPColorBlendState(&blend_state)
        .setPDepthStencilState(&depth_stencil_state)
        .setRenderPass(render_pass)
        .setLayout(layout.get())
    );

    return pipeline(device, {vert_shader, frag_shader}, samplers, std::move(layout), std::move(set_layout),
                    std::move(pl));
}

pipeline create_textured_quad_pipeline(vk::Device device, vk::RenderPass render_pass)
{
    auto vert_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(textured_quad_vert_spv))
        .setPCode(textured_quad_vert_spv)
    );

    auto frag_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(textured_quad_frag_spv))
        .setPCode(textured_quad_frag_spv)
    );

    auto vert_stage = vk::PipelineShaderStageCreateInfo()
                      .setStage(vk::ShaderStageFlagBits::eVertex)
                      .setModule(vert_shader)
                      .setPName("main");

    auto frag_stage = vk::PipelineShaderStageCreateInfo()
                      .setModule(frag_shader)
                      .setStage(vk::ShaderStageFlagBits::eFragment)
                      .setPName("main");

    std::array stages{vert_stage, frag_stage};

    auto input_binding = vk::VertexInputBindingDescription()
        .setStride(sizeof(glm::vec2));

    auto position_attribute = vk::VertexInputAttributeDescription()
                              .setFormat(vk::Format::eR32G32Sfloat)
                              .setLocation(0);

    std::array attributes{position_attribute};

    auto input_state = vk::PipelineVertexInputStateCreateInfo()
                       .setVertexBindingDescriptionCount(1)
                       .setPVertexBindingDescriptions(&input_binding)
                       .setVertexAttributeDescriptions(attributes);

    auto assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleStrip);

    auto viewport = vk::Viewport().setWidth(static_cast<float>(WIDTH))
                                  .setY(static_cast<float>(HEIGHT))
                                  .setHeight(-static_cast<float>(HEIGHT))
                                  .setMaxDepth(1.0);
    auto scissor = vk::Rect2D().setExtent(vk::Extent2D(WIDTH, HEIGHT));

    auto viewport_state = vk::PipelineViewportStateCreateInfo()
                          .setViewportCount(1)
                          .setPViewports(&viewport)
                          .setScissorCount(1)
                          .setPScissors(&scissor);

    auto rasterization_state = vk::PipelineRasterizationStateCreateInfo()
        .setLineWidth(1.f);

    auto multisample_state = vk::PipelineMultisampleStateCreateInfo()
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    auto attachment_state = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        );

    auto blend_state = vk::PipelineColorBlendStateCreateInfo()
                       .setAttachmentCount(1)
                       .setPAttachments(&attachment_state);

    auto depth_stencil_state = vk::PipelineDepthStencilStateCreateInfo();

    std::vector samplers{
        device.createSampler(
            vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        )
    };

    auto sampler_binding = vk::DescriptorSetLayoutBinding()
                           .setBinding(0)
                           .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                           .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                           .setImmutableSamplers(samplers);

    std::array bindings = {sampler_binding};

    auto set_layout = device.createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindings(bindings)
    );
    auto layout = device.createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout.get())
    );

    auto pl = device.createGraphicsPipelineUnique(
        nullptr,
        vk::GraphicsPipelineCreateInfo()
        .setStages(stages)
        .setPVertexInputState(&input_state)
        .setPInputAssemblyState(&assembly_state)
        .setPViewportState(&viewport_state)
        .setPRasterizationState(&rasterization_state)
        .setPMultisampleState(&multisample_state)
        .setPColorBlendState(&blend_state)
        .setPDepthStencilState(&depth_stencil_state)
        .setRenderPass(render_pass)
        .setLayout(layout.get())
    );

    return pipeline(device, {vert_shader, frag_shader}, samplers, std::move(layout), std::move(set_layout),
                    std::move(pl));
}

pipeline create_model_pipeline(vk::Device device, vk::RenderPass render_pass)
{
    auto vert_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(model_vert_shader_spv))
        .setPCode(model_vert_shader_spv)
    );

    auto frag_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(model_frag_shader_spv))
        .setPCode(model_frag_shader_spv)
    );

    auto vert_stage = vk::PipelineShaderStageCreateInfo()
                      .setStage(vk::ShaderStageFlagBits::eVertex)
                      .setModule(vert_shader)
                      .setPName("main");

    auto frag_stage = vk::PipelineShaderStageCreateInfo()
                      .setModule(frag_shader)
                      .setStage(vk::ShaderStageFlagBits::eFragment)
                      .setPName("main");

    std::array stages{vert_stage, frag_stage};

    auto input_binding = vk::VertexInputBindingDescription()
        .setStride(sizeof(vertex));

    auto position_attribute = vk::VertexInputAttributeDescription()
                              .setFormat(vk::Format::eR16G16B16Snorm)
                              .setLocation(0)
                              .setOffset(offsetof(vertex, position));

    auto normal_attribute = vk::VertexInputAttributeDescription()
                            .setFormat(vk::Format::eR16G16B16Snorm)
                            .setLocation(1)
                            .setOffset(offsetof(vertex, normal));

    auto color_attribute = vk::VertexInputAttributeDescription()
                           .setFormat(vk::Format::eR8G8B8Unorm)
                           .setLocation(2)
                           .setOffset(offsetof(vertex, color));

    std::array attributes{position_attribute, normal_attribute, color_attribute};

    auto input_state = vk::PipelineVertexInputStateCreateInfo()
                       .setVertexBindingDescriptionCount(1)
                       .setPVertexBindingDescriptions(&input_binding)
                       .setVertexAttributeDescriptions(attributes);

    auto assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport = vk::Viewport()
                    .setWidth(static_cast<float>(WIDTH))
                    .setY(static_cast<float>(HEIGHT))
                    .setHeight(-static_cast<float>(HEIGHT))
                    .setMaxDepth(1.0);
    auto scissor = vk::Rect2D().setExtent(vk::Extent2D(WIDTH, HEIGHT));

    auto viewport_state = vk::PipelineViewportStateCreateInfo()
                          .setViewportCount(1)
                          .setPViewports(&viewport)
                          .setScissorCount(1)
                          .setPScissors(&scissor);

    auto rasterization_state = vk::PipelineRasterizationStateCreateInfo()
        .setLineWidth(1.f);

    auto multisample_state = vk::PipelineMultisampleStateCreateInfo()
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    auto attachment_state = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        );

    auto blend_state = vk::PipelineColorBlendStateCreateInfo()
                       .setAttachmentCount(1)
                       .setPAttachments(&attachment_state);

    auto depth_stencil_state = vk::PipelineDepthStencilStateCreateInfo()
                               .setDepthTestEnable(true)
                               .setDepthWriteEnable(true)
                               .setDepthCompareOp(vk::CompareOp::eLessOrEqual);

    auto binding = vk::DescriptorSetLayoutBinding()
                   .setDescriptorCount(1)
                   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                   .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    auto set_layout = device.createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(1)
        .setPBindings(&binding)
    );
    auto layout = device.createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout.get())
    );

    auto pl = device.createGraphicsPipelineUnique(
        nullptr,
        vk::GraphicsPipelineCreateInfo()
        .setStages(stages)
        .setPVertexInputState(&input_state)
        .setPInputAssemblyState(&assembly_state)
        .setPViewportState(&viewport_state)
        .setPRasterizationState(&rasterization_state)
        .setPMultisampleState(&multisample_state)
        .setPColorBlendState(&blend_state)
        .setPDepthStencilState(&depth_stencil_state)
        .setRenderPass(render_pass)
        .setLayout(layout.get())
    );

    return pipeline(device, {vert_shader, frag_shader}, std::vector<vk::Sampler>(), std::move(layout),
                    std::move(set_layout), std::move(pl));
}

pipeline create_ray_tracing_pipeline(vk::Device device)
{
    auto raygen_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(model_rgen_shader_spv))
        .setPCode(model_rgen_shader_spv)
    );

    auto closest_hit_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(model_rchit_shader_spv))
        .setPCode(model_rchit_shader_spv)
    );

    auto miss_shader = device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(sizeof(model_rmiss_shader_spv))
        .setPCode(model_rmiss_shader_spv)
    );

    auto raygen_stage = vk::PipelineShaderStageCreateInfo()
                        .setStage(vk::ShaderStageFlagBits::eRaygenKHR)
                        .setModule(raygen_shader)
                        .setPName("main");

    auto closest_hit_stage = vk::PipelineShaderStageCreateInfo()
                             .setModule(closest_hit_shader)
                             .setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
                             .setPName("main");

    auto miss_stage = vk::PipelineShaderStageCreateInfo()
                      .setModule(miss_shader)
                      .setStage(vk::ShaderStageFlagBits::eMissKHR)
                      .setPName("main");

    auto uniform_buffer_binding = vk::DescriptorSetLayoutBinding()
                                  .setBinding(0)
                                  .setDescriptorCount(1)
                                  .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                  .setStageFlags(
                                      vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    auto tlas_binding = vk::DescriptorSetLayoutBinding()
                        .setBinding(1)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                        .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    auto image_binding = vk::DescriptorSetLayoutBinding()
                         .setBinding(2)
                         .setDescriptorCount(1)
                         .setDescriptorType(vk::DescriptorType::eStorageImage)
                         .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    auto vertex_buffer_binding = vk::DescriptorSetLayoutBinding()
                                 .setBinding(3)
                                 .setDescriptorCount(1)
                                 .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                 .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    auto index_buffer_binding = vk::DescriptorSetLayoutBinding()
                                .setBinding(4)
                                .setDescriptorCount(1)
                                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    std::array bindings{
        uniform_buffer_binding,
        tlas_binding,
        image_binding,
        vertex_buffer_binding,
        index_buffer_binding,
    };

    auto set_layout = device.createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindings(bindings)
    );
    auto layout = device.createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout.get())
    );

    std::array stages{raygen_stage, miss_stage, closest_hit_stage};

    std::array groups{
        vk::RayTracingShaderGroupCreateInfoNV()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(RAYGEN_SHADER_INDEX)
        .setClosestHitShader(VK_SHADER_UNUSED_KHR)
        .setAnyHitShader(VK_SHADER_UNUSED_KHR)
        .setIntersectionShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoNV()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(MISS_SHADER_INDEX)
        .setClosestHitShader(VK_SHADER_UNUSED_KHR)
        .setAnyHitShader(VK_SHADER_UNUSED_KHR)
        .setIntersectionShader(VK_SHADER_UNUSED_KHR),
        vk::RayTracingShaderGroupCreateInfoNV()
        .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
        .setGeneralShader(VK_SHADER_UNUSED_KHR)
        .setClosestHitShader(CLOSEST_HIT_SHADER_INDEX)
        .setAnyHitShader(VK_SHADER_UNUSED_KHR)
        .setIntersectionShader(VK_SHADER_UNUSED_KHR),
    };

    auto pl = device.createRayTracingPipelineNVUnique(
        nullptr,
        vk::RayTracingPipelineCreateInfoNV()
        .setMaxRecursionDepth(1)
        .setLayout(layout.get())
        .setStages(stages)
        .setGroups(groups)
    );

    return pipeline(device, {raygen_shader, closest_hit_shader, miss_shader}, {}, std::move(layout),
                    std::move(set_layout), std::move(pl));
}

std::unique_ptr<buffer> create_shader_binding_table(vk::PhysicalDevice physical_device, vk::Device device,
                                                    vk::Pipeline pipeline)
{
    auto props = physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesNV
    >();
    auto alignment = props.get<vk::PhysicalDeviceRayTracingPropertiesNV>().shaderGroupBaseAlignment;
    auto bufferSize = GROUP_COUNT * alignment;
    auto sbt = std::make_unique<buffer>(physical_device, device, vk::BufferUsageFlagBits::eRayTracingKHR,
                                        vk::MemoryPropertyFlagBits::eHostVisible |
                                        vk::MemoryPropertyFlagBits::eHostCoherent, bufferSize);

    auto handleSize = props.get<vk::PhysicalDeviceRayTracingPropertiesNV>().shaderGroupHandleSize;
    auto tempBufferSize = GROUP_COUNT * handleSize;
    auto data = std::make_unique<uint8_t[]>(tempBufferSize);
    device.getRayTracingShaderGroupHandlesNV(pipeline, 0, GROUP_COUNT, tempBufferSize, data.get());

    auto ptr = static_cast<uint8_t*>(device.mapMemory(sbt->memory.get(), 0, VK_WHOLE_SIZE));
    memset(ptr, 0xCA, bufferSize);
    memcpy(&ptr[RAYGEN_SHADER_INDEX * alignment], &data[RAYGEN_SHADER_INDEX * handleSize], handleSize);
    memcpy(&ptr[MISS_SHADER_INDEX * alignment], &data[MISS_SHADER_INDEX * handleSize], handleSize);
    memcpy(&ptr[CLOSEST_HIT_SHADER_INDEX * alignment], &data[CLOSEST_HIT_SHADER_INDEX * handleSize], handleSize);
    device.unmapMemory(sbt->memory.get());

    return sbt;
}

pipeline::pipeline(vk::Device device, std::vector<vk::ShaderModule> shader_modules, std::vector<vk::Sampler> samplers,
                   vk::UniquePipelineLayout layout, vk::UniqueDescriptorSetLayout set_layout, vk::UniquePipeline pl)
    : device(device), shader_modules(shader_modules), samplers(samplers), layout(std::move(layout)),
      set_layout(std::move(set_layout)), pl(std::move(pl))
{
}

pipeline::~pipeline()
{
    for (auto sampler : samplers)
    {
        device.destroySampler(sampler);
    }
    for (auto shader_module : shader_modules)
    {
        device.destroyShaderModule(shader_module);
    }
}
