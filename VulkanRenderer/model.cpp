#include "stdafx.h"
#include "model.h"
#include "data_types.h"
#include "pipeline.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

static glm::i16vec3 to_r16g16b16_snorm(float x, float y, float z)
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
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);

    auto scene = importer.ReadFile(
        path,
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices |
        aiProcess_ValidateDataStructure
    );

    assert(scene->mNumMeshes == 1);
    auto mesh = scene->mMeshes[0];

    buffer vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, mesh->mNumVertices * sizeof(vertex));
    auto vertices = reinterpret_cast<vertex*>(device.mapMemory(vertex_buffer.memory, 0, vertex_buffer.size));
    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        vertices[i].position = to_r16g16b16_snorm(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals())
        {
            vertices[i].normal = a2b10g10r10_snorm_pack32(
                glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z))
            );
        }

        if (mesh->HasVertexColors(0))
        {
            vertices[i].color = a2b10g10r10_unorm_pack32(
                glm::vec3(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b)
            );
        }
        else
        {
            vertices[i].color = a2b10g10r10_unorm_pack32(glm::vec3(.5f));
        }
    }
    device.unmapMemory(vertex_buffer.memory);

    assert(mesh->mNumFaces > 0);
    auto index_count = 3 * mesh->mNumFaces;
    buffer index_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer, index_count * sizeof(uint32_t));
    auto indices = reinterpret_cast<uint32_t*>(device.mapMemory(index_buffer.memory, 0, index_buffer.size));
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        auto face = &mesh->mFaces[i];
        assert(face->mNumIndices == 3);
        for (uint32_t j = 0; j < face->mNumIndices; j++)
        {
            indices[3 * i + j] = face->mIndices[j];
        }
    }
    device.unmapMemory(index_buffer.memory);

    return model(uint32_t(index_count), vertex_buffer, index_buffer);
}
