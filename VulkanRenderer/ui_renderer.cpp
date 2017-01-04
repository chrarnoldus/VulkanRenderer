#include "stdafx.h"
#include "ui_renderer.h"

static const uint32_t MAX_VERTEX_COUNT = UINT16_MAX;
static const uint32_t MAX_INDEX_COUNT = UINT16_MAX;

ui_renderer::ui_renderer(vk::PhysicalDevice physical_device, vk::Device device)
    : vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, MAX_VERTEX_COUNT * sizeof(ImDrawVert))
      , index_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer, MAX_INDEX_COUNT * sizeof(uint16_t))
      , indirect_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndirectBuffer, sizeof(VkDrawIndexedIndirectCommand))
{
}

void ui_renderer::update(vk::Device device) const
{
    // TODO something useful
    ImGui::ShowTestWindow();
    ImGui::Render();
    auto draw_data = ImGui::GetDrawData();

    // we're assuming that there's only one texture
    // TODO implement clipping and order draw calls (important because of alpha blending)

    assert(draw_data->Valid);
    assert(draw_data->TotalVtxCount < MAX_VERTEX_COUNT);
    assert(draw_data->TotalIdxCount < MAX_INDEX_COUNT);

    auto indirect = reinterpret_cast<VkDrawIndexedIndirectCommand*>(device.mapMemory(indirect_buffer.memory, 0, indirect_buffer.size));
    indirect->instanceCount = 1;
    indirect->firstInstance = 0;
    indirect->indexCount = draw_data->TotalIdxCount;
    indirect->firstIndex = 0;
    indirect->vertexOffset = 0;
    device.unmapMemory(indirect_buffer.memory);

    auto indices = reinterpret_cast<uint16_t*>(device.mapMemory(index_buffer.memory, 0, index_buffer.size));
    auto vertices = reinterpret_cast<ImDrawVert*>(device.mapMemory(vertex_buffer.memory, 0, vertex_buffer.size));

    for (auto i = 0; i < draw_data->CmdListsCount; i++)
    {
        auto cmd_list = draw_data->CmdLists[i];
        std::copy(cmd_list->IdxBuffer.begin(), cmd_list->IdxBuffer.end(), stdext::unchecked_array_iterator<uint16_t*>(indices));
        std::copy(cmd_list->VtxBuffer.begin(), cmd_list->VtxBuffer.end(), stdext::unchecked_array_iterator<ImDrawVert*>(vertices));
        indices += cmd_list->IdxBuffer.size();
        vertices += cmd_list->VtxBuffer.size();
    }

    device.unmapMemory(vertex_buffer.memory);
    device.unmapMemory(index_buffer.memory);
}

void ui_renderer::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindIndexBuffer(index_buffer.buf, 0, vk::IndexType::eUint16);
    command_buffer.bindVertexBuffers(0, {vertex_buffer.buf}, {0});
    command_buffer.drawIndexedIndirect(indirect_buffer.buf, 0, 1, sizeof(VkDrawIndexedIndirectCommand));
}

void ui_renderer::destroy(vk::Device device) const
{
    indirect_buffer.destroy(device);
    index_buffer.destroy(device);
    vertex_buffer.destroy(device);
}
