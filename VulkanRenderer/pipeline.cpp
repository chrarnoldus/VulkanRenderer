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

static vk::ShaderModule create_shader_module(vk::Device device, size_t size, const uint32_t* code)
{
    return device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(size)
        .setPCode(code)
    );
}

pipeline::pipeline(class vk::Device device, vk::RenderPass render_pass)
{
    vert_shader = create_shader_module(device, sizeof(model_vert_shader_spv), model_vert_shader_spv);
    frag_shader = create_shader_module(device, sizeof(model_frag_shader_spv), model_frag_shader_spv);

    auto vert_stage = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vert_shader)
        .setPName("main");

    auto frag_stage = vk::PipelineShaderStageCreateInfo()
        .setModule(frag_shader)
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setPName("main");

    const auto stage_count = 2u;
    vk::PipelineShaderStageCreateInfo stages[stage_count] = {vert_stage, frag_stage};

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
        .setFormat(vk::Format::eA2B10G10R10UnormPack32)
        .setLocation(2)
        .setOffset(offsetof(vertex, color));

    const uint32_t attribute_count = 3;
    vk::VertexInputAttributeDescription attributes[attribute_count] = {position_attribute, normal_attribute, color_attribute};

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

    set_layout = device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(1)
        .setPBindings(&binding)
    );
    layout = device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&set_layout)
    );

    pl = device.createGraphicsPipeline(
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
}

void pipeline::destroy(vk::Device device) const
{
    device.destroyPipeline(pl);
    device.destroyPipelineLayout(layout);
    device.destroyDescriptorSetLayout(set_layout);
    device.destroyShaderModule(frag_shader);
    device.destroyShaderModule(vert_shader);
}
