#include "stdafx.h"
#include "render_to_window.h"
#include "dimensions.h"
#include "helpers.h"
#include "input_state.h"
#include "model.h"
#include "pipeline.h"
#include "frame.h"
#include "model_renderer.h"
#include "ui_renderer.h"

class vulkanapp
{
    vk::Queue queue;
    vk::CommandPool command_pool;
    model mdl;
    vk::RenderPass render_pass;
    pipeline model_pipeline;
    pipeline ui_pipeline;
    vk::DescriptorPool descriptor_pool;
    vk::Semaphore acquired_semaphore;
    vk::Semaphore rendered_semaphore;
    std::vector<frame> frames;
    vk::SwapchainKHR swapchain;
    image_with_view font_image;
    glm::quat trackball_rotation;
    float camera_distance;

public:
    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, const std::string& model_path);
    void update(vk::Device device, const input_state& input);
    void destroy(vk::Device device) const;
};

static image_with_view load_font_image(vk::PhysicalDevice physical_device, vk::Device device)
{
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
    assert(bytes_per_pixel == 4);
    return load_r8g8b8a8_unorm_texture(physical_device, device, width, height, pixels);
}

vulkanapp::vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, const std::string& model_path)
    : queue(device.getQueue(0, 0))
    , command_pool(device.createCommandPool(vk::CommandPoolCreateInfo()))
    , mdl(read_model(physical_device, device, command_pool, queue, model_path))
    , render_pass(create_render_pass(device, vk::ImageLayout::ePresentSrcKHR))
    , model_pipeline(create_model_pipeline(device, render_pass))
    , ui_pipeline(create_ui_pipeline(device, render_pass))
    , font_image(load_font_image(physical_device, device))
    , camera_distance(2.f)
{
    auto old_image = font_image;
    font_image = font_image.copy_from_host_to_device_for_shader_read(physical_device, device, command_pool, queue);
    queue.waitIdle();
    old_image.destroy(device);

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
        frames.push_back(frame(
            physical_device,
            device,
            command_pool,
            descriptor_pool,
            image,
            render_pass,
            {
                new model_renderer(physical_device, device, descriptor_pool, model_pipeline, mdl),
                new ui_renderer(physical_device, device, descriptor_pool, ui_pipeline, font_image)
            }
        ));
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

    if (!input.ui_want_capture_mouse)
    {
        if (input.left_mouse_button_down)
        {
            // Based on https://www.khronos.org/opengl/wiki/Object_Mouse_Trackball
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

    device.waitForFences({ frame.rendered_fence }, true, UINT64_MAX);
    device.resetFences({ frame.rendered_fence });

    frame.update(device, data);

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

static void initialize_imgui()
{
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(WIDTH), float(HEIGHT));
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
    // TODO clipboard, ime
}

static void error_callback(int error, const char* description)
{
    throw std::runtime_error(description);
}

void render_to_window(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device, const std::string& model_path)
{
    initialize_imgui();

    auto success = glfwInit();
    assert(success);

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan renderer", nullptr, nullptr);
    assert(window);

    VkSurfaceKHR surface;
    auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    assert(result == VK_SUCCESS);

    input_state input(window);
    auto app = vulkanapp(physical_device, device, surface, model_path);
    while (!glfwWindowShouldClose(window))
    {
        input.update();
        app.update(device, input);
    }
    app.destroy(device);

    instance.destroySurfaceKHR(surface);

    glfwTerminate();

    ImGui::Shutdown();
}
