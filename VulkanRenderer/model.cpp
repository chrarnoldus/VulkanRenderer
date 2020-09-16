#include "stdafx.h"
#include "model.h"
#include "data_types.h"
#include "pipeline.h"

#pragma warning( push )
#pragma warning( disable: 4267 )
#include <tinyply.h>
#pragma warning( pop )

model::model(uint32_t vertex_count, uint32_t index_count, std::unique_ptr<buffer> vertex_buffer, std::unique_ptr<buffer> index_buffer)
    : vertex_count(vertex_count), index_count(index_count), vertex_buffer(std::move(vertex_buffer)), index_buffer(std::move(index_buffer))
{
}

void model::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindIndexBuffer(index_buffer->buf.get(), 0, vk::IndexType::eUint32);
    command_buffer.bindVertexBuffers(0, { vertex_buffer->buf.get() }, { 0 });
    command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
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

static std::vector<float> generate_unnormalized_normals(const std::vector<float>& positions, const std::vector<uint32_t>& indices)
{
    std::vector<float> normals(positions.size(), 0.f);
    for (size_t i = 0; i < indices.size() / 3; i++)
    {
        auto a_index = indices[3 * i];
        glm::vec3 a(positions[3 * a_index], positions[3 * a_index + 1], positions[3 * a_index + 2]);
        auto b_index = indices[3 * i + 1];
        glm::vec3 b(positions[3 * b_index], positions[3 * b_index + 1], positions[3 * b_index + 2]);
        auto c_index = indices[3 * i + 2];
        glm::vec3 c(positions[3 * c_index], positions[3 * c_index + 1], positions[3 * c_index + 2]);

        auto weighted_normal = glm::cross(b - a, c - a);
        normals[3 * a_index] += weighted_normal.x;
        normals[3 * a_index + 1] += weighted_normal.y;
        normals[3 * a_index + 2] += weighted_normal.z;
        normals[3 * b_index] += weighted_normal.x;
        normals[3 * b_index + 1] += weighted_normal.y;
        normals[3 * b_index + 2] += weighted_normal.z;
        normals[3 * c_index] += weighted_normal.x;
        normals[3 * c_index + 1] += weighted_normal.y;
        normals[3 * c_index + 2] += weighted_normal.z;
    }
    return normals;
}

static glm::vec4 unitize(const std::vector<float>& positions)
{
    glm::vec3 min(positions[0], positions[1], positions[2]);
    glm::vec3 max(positions[0], positions[1], positions[2]);

    for (size_t i = 0; i < positions.size() / 3; i++)
    {
        glm::vec3 position(positions[3 * i], positions[3 * i + 1], positions[3 * i + 2]);
        min = glm::min(min, position);
        max = glm::max(max, position);
    }

    return glm::vec4(
        (min + max) / 2.f,
        2.f / glm::max(max.x - min.x, glm::max(max.y - min.y, max.z - min.z))
    );
}

model read_model(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue, const std::string& path)
{
    std::ifstream stream(path, std::ios_base::binary);
    tinyply::PlyFile ply_file(stream);

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint8_t> colors;
    std::vector<uint32_t> indices;

    auto vertex_count = ply_file.request_properties_from_element("vertex", { "x","y","z" }, positions);
    ply_file.request_properties_from_element("vertex", { "nx","ny","nz" }, normals);
    ply_file.request_properties_from_element("vertex", { "red","green","blue" }, colors);
    auto face_count = ply_file.request_properties_from_element("face", { "vertex_indices" }, indices, 3);
    ply_file.read(stream);

    assert(positions.size() > 0);
    glm::vec4 transformation(unitize(positions));
    if (normals.size() == 0)
    {
        normals = generate_unnormalized_normals(positions, indices);
    }
    colors.resize(3 * vertex_count, UINT8_MAX);
    assert(indices.size() > 0);

    buffer vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eTransferSrc, HOST_VISIBLE_AND_COHERENT, vertex_count * sizeof(vertex));
    auto vertices = reinterpret_cast<vertex*>(device.mapMemory(vertex_buffer.memory.get(), 0, vertex_buffer.size));
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        vertices[i].position = r16g16b16_snorm(
            (positions[3 * i] - transformation.x) * transformation.w,
            (positions[3 * i + 1] - transformation.y) * transformation.w,
            (positions[3 * i + 2] - transformation.z) * transformation.w
        );

        auto unnormalized_normal = glm::vec3(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]);
        auto normal = glm::length(unnormalized_normal) > 1.e-10f
            ? glm::normalize(unnormalized_normal)
            : unnormalized_normal;

        vertices[i].normal =r16g16b16_snorm(normal.x, normal.y, normal.z);
        vertices[i].color = glm::u8vec3(colors[3 * i], colors[3 * i + 1], colors[3 * i + 2]);
    }
    device.unmapMemory(vertex_buffer.memory.get());
    auto device_vertex_buffer = vertex_buffer.copy_from_host_to_device_for_vertex_input(
        physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, command_pool, queue
    );

    buffer index_buffer(physical_device, device, vk::BufferUsageFlagBits::eTransferSrc, HOST_VISIBLE_AND_COHERENT, indices.size() * sizeof(*indices.data()));
    memcpy(device.mapMemory(index_buffer.memory.get(), 0, index_buffer.size), indices.data(), indices.size() * sizeof(*indices.data()));
    device.unmapMemory(index_buffer.memory.get());
    auto device_index_buffer = index_buffer.copy_from_host_to_device_for_vertex_input(
        physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, command_pool, queue
    );

    queue.waitIdle();

    std::printf("Model loaded: %llu triangles, %.2lf MB\n", face_count, (vertex_buffer.size + index_buffer.size) / (1024. * 1024.));
    return model(vertex_count, uint32_t(indices.size()), std::move(device_vertex_buffer), std::move(device_index_buffer));
}
