#include "stdafx.h"
#include "pipeline.h"

static std::vector<char> read_file(const char* file_name)
{
    std::ifstream stream(file_name, std::ifstream::binary | std::ifstream::in);
    assert(!stream.fail());

    stream.seekg(0, std::ifstream::end);
    assert(!stream.fail());

    auto size = stream.tellg();
    assert(!stream.fail());

    stream.seekg(0);
    assert(!stream.fail());

    std::vector<char> data(size);
    stream.read(data.data(), size);
    assert(!stream.fail());

    return data;
}

static vk::ShaderModule create_shader_module(vk::Device device, const std::vector<char>& data)
{
    return device.createShaderModule(
        vk::ShaderModuleCreateInfo()
        .setCodeSize(data.size())
        .setPCode(reinterpret_cast<const uint32_t*>(data.data()))
    );
}

pipeline::pipeline(class vk::Device device, vk::RenderPass render_pass, struct buffer uniform_buffer, const char* vert_shader_file_name, const char* frag_shader_file_name)
{
    vert_shader = create_shader_module(device, read_file(vert_shader_file_name));
    frag_shader = create_shader_module(device, read_file(frag_shader_file_name));

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
        .setFormat(vk::Format::eR32G32Sfloat)
        .setLocation(0)
        .setOffset(offsetof(vertex, x));

    auto color_attribute = vk::VertexInputAttributeDescription()
        .setFormat(vk::Format::eR8G8B8Unorm)
        .setLocation(1)
        .setOffset(offsetof(vertex, r));

    const auto attribute_count = 2u;
    vk::VertexInputAttributeDescription attributes[attribute_count] = {position_attribute, color_attribute};

    auto input_state = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(1)
        .setPVertexBindingDescriptions(&input_binding)
        .setVertexAttributeDescriptionCount(attribute_count)
        .setPVertexAttributeDescriptions(attributes);

    auto assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport = vk::Viewport().setWidth(WIDTH).setHeight(HEIGHT).setMaxDepth(1.0);
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

    std::vector<vk::DescriptorPoolSize> sizes({vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 10)});
    descriptor_pool = device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo()
        .setPoolSizeCount(sizes.size())
        .setPPoolSizes(sizes.data())
        .setMaxSets(10)
    );
    descriptor_sets = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&set_layout)
    );

    auto buffer_info = vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf)
        .setRange(uniform_buffer.allocation_size);

    auto write = vk::WriteDescriptorSet()
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setDstSet(descriptor_sets[0])
        .setPBufferInfo(&buffer_info);

    device.updateDescriptorSets({write}, {});

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
        .setRenderPass(render_pass)
        .setLayout(layout)
    );
}

void pipeline::destroy(vk::Device device) const
{
    device.destroyPipeline(pl);
    device.destroyPipelineLayout(layout);
    device.destroyDescriptorSetLayout(set_layout);
    device.destroyDescriptorPool(descriptor_pool);
    device.destroyShaderModule(frag_shader);
    device.destroyShaderModule(vert_shader);
}
