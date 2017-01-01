#include "stdafx.h"
#include "model.h"
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

model read_model(vk::PhysicalDevice physical_device, vk::Device device, const std::string& path)
{
    Assimp::Importer importer;

    auto scene = importer.ReadFile(
        path,
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_ValidateDataStructure
    );

    assert(scene->mNumMeshes == 1);
    auto mesh = scene->mMeshes[0];

    std::vector<vertex> vertices(mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        vertices[i].position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals())
        {
            vertices[i].normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        if (mesh->HasVertexColors(0))
        {
            vertices[i].color = glm::u8vec3(
                uint8_t(255 * mesh->mColors[0][i].r),
                uint8_t(255 * mesh->mColors[0][i].g),
                uint8_t(255 * mesh->mColors[0][i].b)
            );
        }
        else
        {
            vertices[i].color = glm::u8vec3(127);
        }
    }

    buffer vertex_buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, vertices.size() * sizeof(vertex));
    vertex_buffer.update(device, vertices.data());

    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        auto face = &mesh->mFaces[i];
        assert(face->mNumIndices == 3);
        for (uint32_t j = 0; j < face->mNumIndices; j++)
        {
            indices.push_back(face->mIndices[j]);
        }
    }

    assert(indices.size() >= 3);
    buffer index_buffer(physical_device, device, vk::BufferUsageFlagBits::eIndexBuffer, indices.size() * sizeof(uint32_t));
    index_buffer.update(device, indices.data());

    return model(uint32_t(indices.size()), vertex_buffer, index_buffer);
}
