#include "stdafx.h"
#include "vulkanapp.h"

static vk::RenderPass create_render_pass(vk::Device device)
{
    auto attachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    auto color_attachment = vk::AttachmentReference()
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    auto subpass = vk::SubpassDescription()
        .setColorAttachmentCount(1)
        .setPColorAttachments(&color_attachment);

    return device.createRenderPass(
        vk::RenderPassCreateInfo()
        .setAttachmentCount(1)
        .setPAttachments(&attachment)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
    );
}

static buffer create_mesh(vk::PhysicalDevice physical_device, vk::Device device)
{
    const uint32_t vertex_count = 6;
    auto buf = buffer(physical_device, device, vk::BufferUsageFlagBits::eVertexBuffer, vertex_count * sizeof(vertex));

    vertex data[vertex_count];
    data[0] = {-1.f, 1.f, 255, 0, 0};
    data[1] = {1.f, 1.f, 0, 255, 0};
    data[2] = {1.f, -1.f, 0, 0, 255};
    data[3] = {1.f, -1.f, 0, 0, 255};
    data[4] = {-1.f, -1.f, 127, 127, 127};
    data[5] = {-1.f, 1.f, 255, 0, 0};
    buf.update(device, data);

    return buf;
}

static vk::DescriptorPool create_descriptor_pool(vk::Device device)
{
    auto max_ub_count = uint32_t(10);
    std::vector<vk::DescriptorPoolSize> sizes({vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, max_ub_count)});
    return device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo()
        .setPoolSizeCount(uint32_t(sizes.size()))
        .setPPoolSizes(sizes.data())
        .setMaxSets(max_ub_count)
    );
}

vulkanapp::vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface)
    : mesh(create_mesh(physical_device, device))
      , render_pass(create_render_pass(device))
      , pl(pipeline(device, render_pass, "vert.spv", "frag.spv"))
{
    queue = device.getQueue(0, 0);
    descriptor_pool = create_descriptor_pool(device);
}

void vulkanapp::destroy(vk::Device device) const
{
    queue.waitIdle();

    device.destroyDescriptorPool(descriptor_pool);
    pl.destroy(device);
    mesh.destroy(device);
    device.destroyRenderPass(render_pass);
}
