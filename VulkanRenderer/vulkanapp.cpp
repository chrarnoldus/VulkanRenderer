#include "stdafx.h"
#include "vulkanapp.h"
#include "dimensions.h"

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
    command_pool = device.createCommandPool(vk::CommandPoolCreateInfo());
    descriptor_pool = create_descriptor_pool(device);
    acquired_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
    rendered_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());

    auto supported = physical_device.getSurfaceSupportKHR(0, surface);
    assert(supported);

    auto caps = physical_device.getSurfaceCapabilitiesKHR(surface);

    auto formats = physical_device.getSurfaceFormatsKHR(surface);
    assert(formats[0].format == vk::Format::eB8G8R8A8Unorm);

    swapchain = device.createSwapchainKHR(
        vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(2)
        .setImageFormat(vk::Format::eB8G8R8A8Unorm)
        .setImageExtent(caps.currentExtent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(caps.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(true)
        .setPresentMode(vk::PresentModeKHR::eFifo)
    );

    auto images = device.getSwapchainImagesKHR(swapchain);
    for (auto image : images)
    {
        frames.push_back(frame(physical_device, device, command_pool, descriptor_pool, image, render_pass, pl, mesh));
    }
}

void vulkanapp::update(vk::Device device, double timeInSecords) const
{
    auto current_image = device.acquireNextImageKHR(swapchain, UINT64_MAX, acquired_semaphore, nullptr).value;
    auto frame = frames[current_image];

    device.waitForFences({frame.rendered_fence}, true, UINT64_MAX);
    const double seconds_per_rotation = 4.f;
    auto angle = float(std::fmod(timeInSecords, seconds_per_rotation) / seconds_per_rotation) * glm::two_pi<float>();
    auto camera_distance = 2.5f;
    auto transform =
        glm::perspective(glm::half_pi<float>(), float(WIDTH) / float(HEIGHT), .001f, 100.f)
        *
        glm::lookAt(
            glm::vec3(0.f, 0.f, camera_distance),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, -1.f, 0.f)
        )
        *
        glm::rotate(glm::mat4(1.f), angle, glm::vec3(0.f, 1.f, 0.f));
    frame.uniform_buffer.update(device, &transform);

    device.resetFences({frame.rendered_fence});
    auto wait_dst_stage_mask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    queue.submit({
                     vk::SubmitInfo()
                     .setCommandBufferCount(1)
                     .setPCommandBuffers(&frame.command_buffer)
                     .setPWaitDstStageMask(&wait_dst_stage_mask)
                     .setWaitSemaphoreCount(1)
                     .setPWaitSemaphores(&acquired_semaphore)
                     .setSignalSemaphoreCount(1)
                     .setPSignalSemaphores(&rendered_semaphore)
                 }, frame.rendered_fence);

    queue.presentKHR(
        vk::PresentInfoKHR()
        .setSwapchainCount(1)
        .setPSwapchains(&swapchain)
        .setPImageIndices(&current_image)
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&rendered_semaphore)
    );
}

void vulkanapp::destroy(vk::Device device) const
{
    queue.waitIdle();

    for (auto frame : frames)
    {
        frame.destroy(device);
    }

    device.destroySwapchainKHR(swapchain);
    device.destroySemaphore(rendered_semaphore);
    device.destroySemaphore(acquired_semaphore);
    device.destroyDescriptorPool(descriptor_pool);
    device.destroyCommandPool(command_pool);
    pl.destroy(device);
    device.destroyRenderPass(render_pass);
    mesh.destroy(device);
}
