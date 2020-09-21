#include "stdafx.h"
#include "model.h"
#include "data_types.h"
#include "pipeline.h"

#pragma warning( push )
#pragma warning( disable: 4267 )
#include <tinyply.h>
#pragma warning( pop )

model::model(uint32_t vertex_count, uint32_t index_count, std::unique_ptr<buffer> vertex_buffer,
             std::unique_ptr<buffer> index_buffer)
    : index_count(index_count), vertex_count(vertex_count), vertex_buffer(std::move(vertex_buffer)),
      index_buffer(std::move(index_buffer))
{
}

void model::draw(vk::CommandBuffer command_buffer) const
{
    command_buffer.bindIndexBuffer(index_buffer->buf.get(), 0, vk::IndexType::eUint32);
    command_buffer.bindVertexBuffers(0, {vertex_buffer->buf.get()}, {0});
    command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
}

static glm::i16vec3 r16g16b16_snorm(float x, float y, float z)
{
    assert(x >= -1.f && x <= 1.f);
    assert(y >= -1.f && y <= 1.f);
    assert(z >= -1.f && z <= 1.f);

    return glm::i16vec3(
        static_cast<int16_t>(UINT16_MAX * (x + 1) / 2 + INT16_MIN),
        static_cast<int16_t>(UINT16_MAX * (y + 1) / 2 + INT16_MIN),
        static_cast<int16_t>(UINT16_MAX * (z + 1) / 2 + INT16_MIN)
    );
}

static std::vector<float> generate_unnormalized_normals(const std::span<float>& positions,
                                                        const std::span<uint32_t>& indices)
{
    std::vector<float> normals(positions.size(), 0.f);
    for (size_t i = 0; i < indices.size() / 3; i++)
    {
        const auto a_index = indices[3 * i];
        glm::vec3 a(positions[3 * a_index], positions[3 * a_index + 1], positions[3 * a_index + 2]);
        const auto b_index = indices[3 * i + 1];
        glm::vec3 b(positions[3 * b_index], positions[3 * b_index + 1], positions[3 * b_index + 2]);
        const auto c_index = indices[3 * i + 2];
        glm::vec3 c(positions[3 * c_index], positions[3 * c_index + 1], positions[3 * c_index + 2]);

        const auto weighted_normal = cross(b - a, c - a);
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

static glm::vec4 unitize(const std::span<float>& positions)
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

static std::shared_ptr<tinyply::PlyData> try_request_properties_from_element(
    tinyply::PlyFile& ply_file,
    const std::string& element_key,
    const std::vector<std::string>& property_keys
)
{
    try
    {
        return ply_file.request_properties_from_element(element_key, property_keys);
    }
    catch (...)
    {
        return nullptr;
    }
}

model read_model(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool, vk::Queue queue,
                 const std::string& path)
{
    std::ifstream stream(path, std::ios_base::binary);
    tinyply::PlyFile ply_file;
    ply_file.parse_header(stream);

    auto positionData = ply_file.request_properties_from_element("vertex", {"x", "y", "z"});
    auto normalData = try_request_properties_from_element(ply_file, "vertex", {"nx", "ny", "nz"});
    auto colorData = try_request_properties_from_element(ply_file, "vertex", {"red", "green", "blue"});
    auto indexData = ply_file.request_properties_from_element("face", {"vertex_indices"});
    ply_file.read(stream);

    assert(positionData->count > 0);
    std::span positions(reinterpret_cast<float*>(positionData->buffer.get()), 3 * positionData->count);
    glm::vec4 transformation(unitize(positions));

    assert(indexData->count > 0);
    std::span indices(reinterpret_cast<uint32_t*>(indexData->buffer.get()), 3 * indexData->count);


    std::vector<float> normals;
    if (normalData)
    {
        std::span span(reinterpret_cast<float*>(normalData->buffer.get()), 3 * normalData->count);
        normals.assign(span.begin(), span.end());
    }
    else
    {
        normals = generate_unnormalized_normals(positions, indices);
    }

    std::vector<uint8_t> colors(3 * positionData->count, UINT8_MAX);
    if (colorData)
    {
        std::span span(colorData->buffer.get(), 3 * colorData->count);
        colors.assign(span.begin(), span.end());
    }

    buffer vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eTransferSrc, HOST_VISIBLE_AND_COHERENT,
                         positionData->count * sizeof(vertex));
    auto* vertices = static_cast<vertex*>(device.mapMemory(vertex_buffer.memory.get(), 0, vertex_buffer.size));
    for (uint32_t i = 0; i < positionData->count; i++)
    {
        vertices[i].position = r16g16b16_snorm(
            (positions[3 * i] - transformation.x) * transformation.w,
            (positions[3 * i + 1] - transformation.y) * transformation.w,
            (positions[3 * i + 2] - transformation.z) * transformation.w
        );

        auto unnormalized_normal = glm::vec3(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]);
        auto normal = length(unnormalized_normal) > 1.e-10f
                          ? normalize(unnormalized_normal)
                          : unnormalized_normal;

        vertices[i].normal = r16g16b16_snorm(normal.x, normal.y, normal.z);
        vertices[i].color = glm::u8vec3(colors[3 * i], colors[3 * i + 1], colors[3 * i + 2]);
    }
    device.unmapMemory(vertex_buffer.memory.get());
    auto device_vertex_buffer = vertex_buffer.copy_from_host_to_device_for_vertex_input(
        physical_device, device,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eTransferDst, command_pool, queue
    );

    buffer index_buffer(physical_device, device, vk::BufferUsageFlagBits::eTransferSrc, HOST_VISIBLE_AND_COHERENT,
                        indices.size() * sizeof(*indices.data()));
    memcpy(device.mapMemory(index_buffer.memory.get(), 0, index_buffer.size), indices.data(),
           indices.size() * sizeof(*indices.data()));
    device.unmapMemory(index_buffer.memory.get());
    auto device_index_buffer = index_buffer.copy_from_host_to_device_for_vertex_input(
        physical_device, device,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eTransferDst, command_pool, queue
    );

    queue.waitIdle();

    std::printf("Model loaded: %llu triangles, %.2lf MB\n", positionData->count / 3,
                (vertex_buffer.size + index_buffer.size) / (1024. * 1024.));
    return model(positionData->count, static_cast<uint32_t>(indices.size()), std::move(device_vertex_buffer),
                 std::move(device_index_buffer));
}
