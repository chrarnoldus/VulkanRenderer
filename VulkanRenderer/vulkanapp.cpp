#include "stdafx.h"
#include "vulkanapp.h"
#include "data_types.h"
#include "dimensions.h"
#include "model.h"

static vk::RenderPass create_render_pass(vk::Device device)
{
    auto attachment0 = vk::AttachmentDescription()
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

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

    const uint32_t attachment_count = 2;
    vk::AttachmentDescription attachments[attachment_count] = {attachment0,attachment1};

    return device.createRenderPass(
        vk::RenderPassCreateInfo()
        .setAttachmentCount(attachment_count)
        .setPAttachments(attachments)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
    );
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

vulkanapp::vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, model mdl)
    : mdl(mdl)
      , render_pass(create_render_pass(device))
      , pl(pipeline(device, render_pass))
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
        frames.push_back(frame(physical_device, device, command_pool, descriptor_pool, image, render_pass, pl, mdl));
    }
}

void vulkanapp::update(vk::Device device, float camera_distance, const glm::mat4& rotation) const
{
    auto current_image = device.acquireNextImageKHR(swapchain, UINT64_MAX, acquired_semaphore, nullptr).value;
    auto frame = frames[current_image];

    device.waitForFences({frame.rendered_fence}, true, UINT64_MAX);
    uniform_data data;
    data.projection = glm::perspective(glm::half_pi<float>(), float(WIDTH) / float(HEIGHT), .001f, 100.f);
    data.model_view =
        glm::lookAt(
            glm::vec3(0.f, 0.f, camera_distance),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, -1.f, 0.f)
        )
        *
        rotation;
    frame.uniform_buffer.update(device, &data);

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
    mdl.destroy(device);
}
