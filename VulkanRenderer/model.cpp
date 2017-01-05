#include "stdafx.h"
#include "model.h"
#include "data_types.h"
#include "pipeline.h"

model::model(uint32_t vertex_count, buffer vertex_buffer, buffer index_buffer)
    : index_count(vertex_count), vertex_buffer(vertex_buffer), index_buffer(index_buffer)
{
}

void model::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindIndexBuffer(index_buffer.buf, 0, vk::IndexType::eUint32);
    command_buffer.bindVertexBuffers(0, {vertex_buffer.buf}, {0});
    command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
}

void model::destroy(vk::Device device) const
{
    vertex_buffer.destroy(device);
    index_buffer.destroy(device);
}

static glm::i16vec3 r16g16b16_snorm(float x, float y, float z)
{
    assert(x >= -1.f && x <= 1.f);
    assert(y >= -1.f && y <= 1.f);
    assert(z >= -1.f && z <= 1.f);

    return glm::i16vec3(
        int16_t(UINT16_MAX * (x + 1) / 2 + INT16_MIN),
        int16_t(UINT16_MAX * (y + 1) / 2 + INT16_MIN),
        int16_t(UINT16_MAX * (z + 1) / 2 + INT16_MIN)
    );
}

model read_model(vk::PhysicalDevice physical_device, vk::Device device, const std::string& path)
{
    std::printf("Importing model using tinyply\n");
    std::ifstream stream(path, std::ios_base::binary);

    tinyply::PlyFile ply_file(stream);

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint8_t> colors;
    std::vector<uint32_t> indices;

    auto vertex_count = ply_file.request_properties_from_element("vertex", {"x","y","z"}, positions);
    ply_file.request_properties_from_element("vertex", {"nx","ny","nz"}, normals);
    ply_file.request_properties_from_element("vertex", {"red","green","blue", "alpha"}, colors);
    ply_file.request_properties_from_element("face", {"vertex_indices"}, indices, 3);
    ply_file.read(stream);

    std::printf("Loading model to buffers\n");

    buffer vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, vertex_count * sizeof(vertex));
    auto vertices = reinterpret_cast<vertex*>(device.mapMemory(vertex_buffer.memory, 0, vertex_buffer.size));
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        vertices[i].position = r16g16b16_snorm(positions[3 * i], positions[3 * i + 1], positions[3 * i + 2]);
        vertices[i].normal = a2b10g10r10_snorm_pack32(
            glm::normalize(glm::vec3(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]))
        );
        vertices[i].color = glm::u8vec3(colors[4 * i], colors[4 * i + 1], colors[4 * i + 2]);
    }
    device.unmapMemory(vertex_buffer.memory);

    buffer index_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer, indices.size() * sizeof(*indices.data()));
    memcpy(device.mapMemory(index_buffer.memory, 0, index_buffer.size), indices.data(), indices.size() * sizeof(*indices.data()));
    device.unmapMemory(index_buffer.memory);

    std::printf("Model loaded: %.2lf MB\n", (vertex_buffer.size + index_buffer.size) / (1024. * 1024.));
    return model(uint32_t(indices.size()), vertex_buffer, index_buffer);
}
