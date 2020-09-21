#include "stdafx.h"

#include "model.h"
#include "helpers.h"
#include "pipeline.h"
#include "dimensions.h"
#include "image_with_view.h"
#include "frame.h"
#include "model_renderer.h"

vk::UniqueRenderPass create_render_pass(vk::Device device, vk::Format color_format, vk::ImageLayout final_layout)
{
    auto attachment0 = vk::AttachmentDescription()
                       .setFormat(color_format)
                       .setSamples(vk::SampleCountFlagBits::e1)
                       .setLoadOp(vk::AttachmentLoadOp::eClear)
                       .setStoreOp(vk::AttachmentStoreOp::eStore)
                       .setInitialLayout(vk::ImageLayout::eUndefined)
                       .setFinalLayout(final_layout);

    auto attachment1 = vk::AttachmentDescription()
                       .setFormat(vk::Format::eD24UnormS8Uint)
                       .setSamples(vk::SampleCountFlagBits::e1)
                       .setLoadOp(vk::AttachmentLoadOp::eClear)
                       .setStoreOp(vk::AttachmentStoreOp::eStore)
                       .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                       .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                       .setInitialLayout(vk::ImageLayout::eUndefined)
                       .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto color_attachment = vk::AttachmentReference()
                            .setAttachment(0)
                            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    auto depth_attachment = vk::AttachmentReference()
                            .setAttachment(1)
                            .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto subpass = vk::SubpassDescription()
                   .setColorAttachmentCount(1)
                   .setPColorAttachments(&color_attachment)
                   .setPDepthStencilAttachment(&depth_attachment);

    std::array attachments{attachment0, attachment1};

    return device.createRenderPassUnique(
        vk::RenderPassCreateInfo()
        .setAttachments(attachments)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
    );
}

vk::UniqueDescriptorPool create_descriptor_pool(vk::Device device)
{
    auto max_count_per_type = static_cast<uint32_t>(10);
    std::array sizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, max_count_per_type),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, max_count_per_type),
    };
    return device.createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo()
        .setPoolSizes(sizes)
        .setMaxSets(max_count_per_type * sizes.size())
    );
}

void render_to_image(
    vk::PhysicalDevice physical_device,
    vk::Device device,
    const std::string& model_path,
    const std::string& image_path,
    const glm::vec3& camera_position,
    const glm::vec3& camera_up
)
{
    auto queue = device.getQueue(0, 0);
    auto command_pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo());
    auto model = read_model(physical_device, device, command_pool.get(), queue, model_path);
    auto render_pass = create_render_pass(device, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferSrcOptimal);
    auto pipeline = create_model_pipeline(device, render_pass.get());
    auto descriptor_pool = create_descriptor_pool(device);

    auto device_image = image_with_memory(
        physical_device,
        device,
        WIDTH,
        HEIGHT,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        vk::ImageTiling::eOptimal,
        vk::ImageLayout::eUndefined,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );

    std::vector<std::unique_ptr<renderer>> renderers;
    renderers.emplace_back(new model_renderer(physical_device, device, descriptor_pool.get(), &pipeline, &model));

    frame frame(physical_device, device, command_pool.get(), descriptor_pool.get(), device_image.image.get(),
                vk::Format::eR8G8B8A8Unorm, render_pass.get(),
                std::move(renderers));

    model_uniform_data data;
    data.projection = glm::perspective(glm::half_pi<float>(), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT),
                                       .001f, 100.f);
    data.model_view = lookAt(camera_position, glm::vec3(0.f, 0.f, 0.f), camera_up);
    frame.update(data);

    device.resetFences({frame.rendered_fence.get()});
    queue.submit({
                     vk::SubmitInfo()
                     .setCommandBufferCount(1)
                     .setPCommandBuffers(&frame.command_buffer)
                 }, frame.rendered_fence.get());
    device.waitForFences({frame.rendered_fence.get()}, true, UINT64_MAX);

    auto host_image = device_image.copy_from_device_to_host(physical_device, device, command_pool.get(), queue);
    queue.waitIdle();

    auto* ptr = reinterpret_cast<uint8_t*>(device.mapMemory(host_image->memory.get(), 0,
                                                            4 * host_image->width * host_image->height));
    // TODO fix channels
    lodepng::encode(image_path, ptr, host_image->width, host_image->height);
    device.unmapMemory(host_image->memory.get());
}
