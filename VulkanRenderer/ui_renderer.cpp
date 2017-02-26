#include "stdafx.h"
#include "ui_renderer.h"
#include "data_types.h"
#include "dimensions.h"

static const uint32_t MAX_VERTEX_COUNT = UINT16_MAX;
static const uint32_t MAX_INDEX_COUNT = UINT16_MAX;

ui_renderer::ui_renderer(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool, pipeline ui_pipeline, image_with_view font_image)
    : vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, HOST_VISIBLE_AND_COHERENT, MAX_VERTEX_COUNT * sizeof(ImDrawVert))
    , index_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer, HOST_VISIBLE_AND_COHERENT, MAX_INDEX_COUNT * sizeof(uint16_t))
    , indirect_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndirectBuffer, HOST_VISIBLE_AND_COHERENT, MAX_UI_DRAW_COUNT * sizeof(VkDrawIndexedIndirectCommand))
    , uniform_buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, HOST_VISIBLE_AND_COHERENT, sizeof(ui_uniform_data))
    , ui_pipeline(ui_pipeline)
    , font_image(font_image)
{
    auto set_layouts = { ui_pipeline.set_layout };
    descriptor_set = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(uint32_t(set_layouts.size()))
        .setPSetLayouts(set_layouts.begin())
    )[0];


    auto ui_ub_info = vk::DescriptorBufferInfo()
        .setBuffer(uniform_buffer.buf)
        .setRange(uniform_buffer.size);

    auto ui_ub_write_description = vk::WriteDescriptorSet()
        .setDstBinding(0)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setDstSet(descriptor_set)
        .setPBufferInfo(&ui_ub_info);

    auto font_image_view_info = vk::DescriptorImageInfo()
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setImageView(font_image.image_view);

    auto font_image_write_descriptor_set = vk::WriteDescriptorSet()
        .setDstBinding(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setDescriptorCount(1)
        .setDstSet(descriptor_set)
        .setPImageInfo(&font_image_view_info);

    device.updateDescriptorSets({ ui_ub_write_description, font_image_write_descriptor_set }, {});
}

void ui_renderer::update(vk::Device device, model_uniform_data model_uniform_data) const
{
    // TODO something useful
    ImGui::ShowTestWindow();
    ImGui::Render();
    auto draw_data = ImGui::GetDrawData();

    // we're assuming that there's only one texture

    assert(draw_data->Valid);
    assert(draw_data->TotalVtxCount < MAX_VERTEX_COUNT);
    assert(draw_data->TotalIdxCount < MAX_INDEX_COUNT);

    auto indices = reinterpret_cast<uint16_t*>(device.mapMemory(index_buffer.memory, 0, index_buffer.size));
    auto vertices = reinterpret_cast<ImDrawVert*>(device.mapMemory(vertex_buffer.memory, 0, vertex_buffer.size));
    auto indirect = reinterpret_cast<VkDrawIndexedIndirectCommand*>(device.mapMemory(indirect_buffer.memory, 0, indirect_buffer.size));
    memset(indirect, 0, indirect_buffer.size);

    auto uniform = reinterpret_cast<ui_uniform_data*>(device.mapMemory(uniform_buffer.memory, 0, uniform_buffer.size));
    uniform->screen_width = float(WIDTH);
    uniform->screen_height = float(HEIGHT);

    uint32_t indirect_index = 0, list_first_index = 0, list_first_vertex = 0;
    for (auto i = 0; i < draw_data->CmdListsCount; i++)
    {
        auto cmd_list = draw_data->CmdLists[i];

        memcpy(indices + list_first_index, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.size() * sizeof(*cmd_list->IdxBuffer.Data));
        memcpy(vertices + list_first_vertex, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.size() * sizeof(*cmd_list->VtxBuffer.Data));

        uint32_t cmd_first_index = 0;
        for (auto& cmd : cmd_list->CmdBuffer)
        {
            assert(indirect_index < MAX_UI_DRAW_COUNT);

            auto indirect_command = &indirect[indirect_index];
            indirect_command->instanceCount = 1;
            indirect_command->firstInstance = indirect_index;
            indirect_command->indexCount = cmd.ElemCount;
            indirect_command->firstIndex = list_first_index + cmd_first_index;
            indirect_command->vertexOffset = list_first_vertex;
            uniform->clip_rects[indirect_index] = cmd.ClipRect;

            cmd_first_index += cmd.ElemCount;
            indirect_index++;
        }

        list_first_index += cmd_list->IdxBuffer.size();
        list_first_vertex += cmd_list->VtxBuffer.size();
    }

    device.unmapMemory(uniform_buffer.memory);
    device.unmapMemory(indirect_buffer.memory);
    device.unmapMemory(vertex_buffer.memory);
    device.unmapMemory(index_buffer.memory);
}

void ui_renderer::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ui_pipeline.layout, 0, descriptor_set, {});
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, ui_pipeline.pl);

    command_buffer.bindIndexBuffer(index_buffer.buf, 0, vk::IndexType::eUint16);
    command_buffer.bindVertexBuffers(0, { vertex_buffer.buf }, { 0 });
    command_buffer.drawIndexedIndirect(indirect_buffer.buf, 0, MAX_UI_DRAW_COUNT, sizeof(VkDrawIndexedIndirectCommand));
}

void ui_renderer::destroy(vk::Device device) const
{
    uniform_buffer.destroy(device);
    indirect_buffer.destroy(device);
    index_buffer.destroy(device);
    vertex_buffer.destroy(device);
}
