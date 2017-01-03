#include "stdafx.h"
#include "ui_renderer.h"

static const uint32_t MAX_VERTEX_COUNT = UINT16_MAX;
static const uint32_t MAX_INDEX_COUNT = UINT16_MAX;
static const uint32_t MAX_DRAW_COUNT = 64;
static const VkDrawIndexedIndirectCommand EMPTY_COMMAND = {0,0,0,0,0};

static image2d load_font_image(vk::PhysicalDevice physical_device, vk::Device device)
{
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
    assert(bytes_per_pixel == 4);
    return load_r8g8b8a8_unorm_texture(physical_device, device, width, height, pixels);
}

ui_renderer::ui_renderer(vk::PhysicalDevice physical_device, vk::Device device)
    : vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, MAX_VERTEX_COUNT * sizeof(ImDrawVert))
      , index_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer, MAX_INDEX_COUNT * sizeof(uint16_t))
      , indirect_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndirectBuffer, MAX_DRAW_COUNT * sizeof(VkDrawIndexedIndirectCommand))
      , font_image(load_font_image(physical_device, device))
{
}

void ui_renderer::update(vk::Device device) const
{
    ImGui::NewFrame();
    // TODO something useful
    ImGui::ShowTestWindow();
    ImGui::Render();
    auto draw_data = ImGui::GetDrawData();

    assert(draw_data->Valid);
    assert(draw_data->CmdListsCount < MAX_DRAW_COUNT);
    assert(draw_data->TotalIdxCount < MAX_INDEX_COUNT);
    assert(draw_data->Valid);

    auto indirect = reinterpret_cast<VkDrawIndexedIndirectCommand*>(device.mapMemory(indirect_buffer.memory, 0, indirect_buffer.size));

    auto indices = reinterpret_cast<uint16_t*>(device.mapMemory(index_buffer.memory, 0, index_buffer.size));
    auto vertices = reinterpret_cast<ImDrawVert*>(device.mapMemory(vertex_buffer.memory, 0, vertex_buffer.size));

    auto first_index = uint32_t(0);
    auto vertex_offset = uint32_t(0);
    for (auto i = 0; i < draw_data->CmdListsCount; i++)
    {
        auto& cmd_list = draw_data->CmdLists[i];
        indirect[i].instanceCount = 1;
        indirect[i].firstInstance = 0;
        indirect[i].indexCount = cmd_list->IdxBuffer.size();
        indirect[i].firstIndex = first_index;
        indirect[i].vertexOffset = vertex_offset;

        std::copy(cmd_list->IdxBuffer.begin(), cmd_list->IdxBuffer.end(), stdext::unchecked_array_iterator<uint16_t*>(indices + first_index));
        std::copy(cmd_list->VtxBuffer.begin(), cmd_list->VtxBuffer.end(), stdext::unchecked_array_iterator<ImDrawVert*>(vertices + vertex_offset));

        first_index += cmd_list->IdxBuffer.size();
        vertex_offset += cmd_list->VtxBuffer.size();
    }

    device.unmapMemory(vertex_buffer.memory);
    device.unmapMemory(index_buffer.memory);

    std::fill(indirect + draw_data->CmdListsCount, indirect + MAX_DRAW_COUNT, EMPTY_COMMAND);
    device.unmapMemory(indirect_buffer.memory);
}

void ui_renderer::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindIndexBuffer(index_buffer.buf, 0, vk::IndexType::eUint16);
    command_buffer.bindVertexBuffers(0, {vertex_buffer.buf}, {0});
    command_buffer.drawIndexedIndirect(indirect_buffer.buf, 0, MAX_DRAW_COUNT, sizeof(VkDrawIndexedIndirectCommand));
}

void ui_renderer::destroy(vk::Device device) const
{
    font_image.destroy(device);
    indirect_buffer.destroy(device);
    index_buffer.destroy(device);
    vertex_buffer.destroy(device);
}
