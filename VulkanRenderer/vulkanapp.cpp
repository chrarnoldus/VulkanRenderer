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
    auto max_count_per_type = uint32_t(10);
    std::vector<vk::DescriptorPoolSize> sizes({
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, max_count_per_type),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, max_count_per_type),
    });
    return device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo()
        .setPoolSizeCount(uint32_t(sizes.size()))
        .setPPoolSizes(sizes.data())
        .setMaxSets(max_count_per_type * uint32_t(sizes.size()))
    );
}

static image2d load_font_image(vk::PhysicalDevice physical_device, vk::Device device)
{
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
    assert(bytes_per_pixel == 4);
    return load_r8g8b8a8_unorm_texture(physical_device, device, width, height, pixels);
}

vulkanapp::vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, model mdl)
    : mdl(mdl)
      , render_pass(create_render_pass(device))
      , model_pipeline(create_model_pipeline(device, render_pass))
      , ui_pipeline(create_ui_pipeline(device, render_pass))
      , font_image(load_font_image(physical_device, device))
      , camera_distance(2.f)
{
    queue = device.getQueue(0, 0);
    command_pool = device.createCommandPool(vk::CommandPoolCreateInfo());
    font_image.transition_layout_from_preinitialized_to_shader_read_only(device, command_pool, queue);

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
        frames.push_back(frame(physical_device, device, command_pool, descriptor_pool, image, render_pass, model_pipeline, ui_pipeline, mdl, font_image));
    }
}

static glm::vec3 get_trackball_position(glm::vec2 mouse_position)
{
    glm::vec2 origin(WIDTH / 2.f, HEIGHT / 2.f);
    auto radius = glm::min(WIDTH, HEIGHT) / 2.f;

    auto xy = glm::vec2(mouse_position.x, HEIGHT - mouse_position.y - 1) - origin;

    if (glm::dot(xy, xy) <= radius * radius / 2.f)
    {
        // Sphere
        auto z = glm::sqrt(radius * radius - glm::dot(xy, xy));
        return glm::vec3(xy, z);
    }
    else
    {
        // Hyperbola
        auto z = (radius * radius / 2.f) / glm::length(xy);
        return glm::vec3(xy, z);
    }
}

void vulkanapp::update(vk::Device device, const input_state& input)
{
    auto current_image = device.acquireNextImageKHR(swapchain, UINT64_MAX, acquired_semaphore, nullptr).value;
    auto& frame = frames[current_image];

    device.waitForFences({frame.rendered_fence}, true, UINT64_MAX);
    device.resetFences({frame.rendered_fence});

    if (!input.ui_wants_mouse)
    {
        if (input.left_mouse_button_down)
        {
            auto
                v1 = glm::normalize(get_trackball_position(input.previous_mouse_position)),
                v2 = glm::normalize(get_trackball_position(input.current_mouse_position));
            trackball_rotation = glm::quat(v1, v2) * trackball_rotation;
        }
        camera_distance *= float(1 - .1 * input.scroll_amount);
    }

    model_uniform_data data;
    data.projection = glm::perspective(glm::half_pi<float>(), float(WIDTH) / float(HEIGHT), .001f, 100.f);
    data.model_view =
        glm::lookAt(
            glm::vec3(0.f, 0.f, camera_distance),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, -1.f, 0.f)
        )
        *
        glm::mat4_cast(trackball_rotation);
    frame.uniform_buffer.update(device, &data);

    frame.ui.update(device);

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

    font_image.destroy(device);
    device.destroySwapchainKHR(swapchain);
    device.destroySemaphore(rendered_semaphore);
    device.destroySemaphore(acquired_semaphore);
    device.destroyDescriptorPool(descriptor_pool);
    device.destroyCommandPool(command_pool);
    ui_pipeline.destroy(device);
    model_pipeline.destroy(device);
    device.destroyRenderPass(render_pass);
    mdl.destroy(device);
}
