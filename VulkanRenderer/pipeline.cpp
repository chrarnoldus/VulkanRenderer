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

static uint32_t ui_vert_shader_spv[] = {
#include "ui.vert.num"
};

static uint32_t ui_frag_shader_spv[] = {
#include "ui.frag.num"
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

    const auto stage_count = 2u;
    vk::PipelineShaderStageCreateInfo stages[stage_count] = { vert_stage, frag_stage };

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

    const uint32_t attribute_count = 3;
    vk::VertexInputAttributeDescription attributes[attribute_count] = { position_attribute, uv_attribute, color_attribute };

    auto input_state = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(1)
        .setPVertexBindingDescriptions(&input_binding)
        .setVertexAttributeDescriptionCount(attribute_count)
        .setPVertexAttributeDescriptions(attributes);

    auto assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport = vk::Viewport().setWidth(float(WIDTH)).setHeight(float(HEIGHT)).setMaxDepth(1.0);
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

    auto samplers = std::vector<vk::Sampler>({
        device.createSampler(
            vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        ) });


    auto uniform_binding = vk::DescriptorSetLayoutBinding()
        .setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    auto sampler_binding = vk::DescriptorSetLayoutBinding()
        .setBinding(1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(samplers.data());

    auto bindings = { uniform_binding, sampler_binding };

    auto set_layout = device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(uint32_t(bindings.size()))
        .setPBindings(bindings.begin())
    );
    auto layout = device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout)
    );

    auto pl = device.createGraphicsPipeline(
        nullptr,
        vk::GraphicsPipelineCreateInfo()
        .setStageCount(stage_count)
        .setPStages(stages)
        .setPVertexInputState(&input_state)
        .setPInputAssemblyState(&assembly_state)
        .setPViewportState(&viewport_state)
        .setPRasterizationState(&rasterization_state)
        .setPMultisampleState(&multisample_state)
        .setPColorBlendState(&blend_state)
        .setPDepthStencilState(&depth_stencil_state)
        .setRenderPass(render_pass)
        .setLayout(layout)
    );

    return pipeline(vert_shader, frag_shader, samplers, layout, set_layout, pl);
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

    const auto stage_count = 2u;
    vk::PipelineShaderStageCreateInfo stages[stage_count] = { vert_stage, frag_stage };

    auto input_binding = vk::VertexInputBindingDescription()
        .setStride(sizeof(vertex));

    auto position_attribute = vk::VertexInputAttributeDescription()
        .setFormat(vk::Format::eR16G16B16Snorm)
        .setLocation(0)
        .setOffset(offsetof(vertex, position));

    auto normal_attribute = vk::VertexInputAttributeDescription()
        .setFormat(vk::Format::eA2B10G10R10SnormPack32)
        .setLocation(1)
        .setOffset(offsetof(vertex, normal));

    auto color_attribute = vk::VertexInputAttributeDescription()
        .setFormat(vk::Format::eR8G8B8Unorm)
        .setLocation(2)
        .setOffset(offsetof(vertex, color));

    const uint32_t attribute_count = 3;
    vk::VertexInputAttributeDescription attributes[attribute_count] = { position_attribute, normal_attribute, color_attribute };

    auto input_state = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(1)
        .setPVertexBindingDescriptions(&input_binding)
        .setVertexAttributeDescriptionCount(attribute_count)
        .setPVertexAttributeDescriptions(attributes);

    auto assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport = vk::Viewport().setWidth(float(WIDTH)).setHeight(float(HEIGHT)).setMaxDepth(1.0);
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

    auto set_layout = device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(1)
        .setPBindings(&binding)
    );
    auto layout = device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout)
    );

    auto pl = device.createGraphicsPipeline(
        nullptr,
        vk::GraphicsPipelineCreateInfo()
        .setStageCount(stage_count)
        .setPStages(stages)
        .setPVertexInputState(&input_state)
        .setPInputAssemblyState(&assembly_state)
        .setPViewportState(&viewport_state)
        .setPRasterizationState(&rasterization_state)
        .setPMultisampleState(&multisample_state)
        .setPColorBlendState(&blend_state)
        .setPDepthStencilState(&depth_stencil_state)
        .setRenderPass(render_pass)
        .setLayout(layout)
    );

    return pipeline(vert_shader, frag_shader, std::vector<vk::Sampler>(), layout, set_layout, pl);
}

pipeline::pipeline(vk::ShaderModule vert_shader, vk::ShaderModule frag_shader, std::vector<vk::Sampler> samplers, vk::PipelineLayout layout, vk::DescriptorSetLayout set_layout, vk::Pipeline pl)
    : vert_shader(vert_shader), frag_shader(frag_shader), samplers(samplers), layout(layout), set_layout(set_layout), pl(pl)
{
}

void pipeline::destroy(vk::Device device) const
{
    device.destroyPipeline(pl);
    device.destroyPipelineLayout(layout);
    device.destroyDescriptorSetLayout(set_layout);
    for (auto sampler : samplers)
    {
        device.destroySampler(sampler);
    }
    device.destroyShaderModule(frag_shader);
    device.destroyShaderModule(vert_shader);
}
