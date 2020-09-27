#include "stdafx.h"
#include "render_to_window.h"
#include "vulkan_context.h"
#include "input_state.h"
#include "model.h"
#include "pipeline.h"
#include "frame.h"
#include "frame_set.h"
#include "model_renderer.h"
#include "ray_tracer.h"
#include "ray_tracing_model.h"
#include "ray_tracing_renderer.h"
#include "swapchain.h"

class vulkanapp
{
    vulkan_context context;
    vk::SurfaceKHR surface;
    model mdl;
    pipeline textured_quad_pipeline;
    pipeline model_pipeline;
    pipeline ui_pipeline;
    vk::UniqueSemaphore acquired_semaphore;
    swapchain current_swapchain;
    image_with_view font_image;
    std::unique_ptr<ray_tracer> ray_tracer;
    frame_set default_frame_set;
    glm::quat trackball_rotation;
    float camera_distance;

    void recreate_swapchain(vk::Extent2D framebuffer_size);
    model_renderer* create_model_renderer(vk::Extent2D framebuffer_size);

public:
    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface,
              vk::Extent2D framebuffer_size, const std::string& model_path);
    void update(vk::Device device, const input_state& input);
    ~vulkanapp();
};

static image_with_view load_font_image(vk::PhysicalDevice physical_device, vk::Device device,
                                       vk::CommandPool command_pool, vk::Queue queue)
{
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
    assert(bytes_per_pixel == 4);
    const auto host_image = load_r8g8b8a8_unorm_texture(physical_device, device, width, height, pixels);
    auto device_image = image_with_view(
        device,
        host_image->copy_from_host_to_device_for_shader_read(physical_device, device, command_pool, queue)
    );
    queue.waitIdle();
    return device_image;
}

vulkanapp::vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface,
                     vk::Extent2D framebuffer_size, const std::string& model_path)
    : context(physical_device, device)
      , surface(surface)
      , mdl(read_model(physical_device, device, context.command_pool.get(), context.queue, model_path))
      , textured_quad_pipeline(create_textured_quad_pipeline(device, context.render_pass.get()))
      , model_pipeline(create_model_pipeline(device, context.render_pass.get()))
      , ui_pipeline(create_ui_pipeline(device, context.render_pass.get()))
      , acquired_semaphore(device.createSemaphoreUnique(vk::SemaphoreCreateInfo()))
      , current_swapchain(physical_device, device, surface, nullptr)
      , font_image(load_font_image(physical_device, device, context.command_pool.get(), context.queue))
      , ray_tracer(context.is_ray_tracing_supported
                       ? std::make_unique<class ray_tracer>(context, current_swapchain.images, framebuffer_size, &mdl,
                                                            &ui_pipeline,
                                                            &font_image)
                       : nullptr)
      , default_frame_set(create_frame_set(context, framebuffer_size, current_swapchain.images, [&]()
      {
          return create_model_renderer(framebuffer_size);
      }, &ui_pipeline, &font_image))
      , camera_distance(2.f)
{
}

void vulkanapp::recreate_swapchain(vk::Extent2D framebuffer_size)
{
    context.queue.waitIdle();
    current_swapchain = swapchain(context.physical_device, context.device, surface, current_swapchain.handle.get());
    default_frame_set = create_frame_set(context, framebuffer_size, current_swapchain.images, [&]()
    {
        return create_model_renderer(framebuffer_size);
    }, &ui_pipeline, &font_image);

    if (ray_tracer)
    {
        ray_tracer->recreate_swapchain(context, framebuffer_size, current_swapchain.images);
    }
}

model_renderer* vulkanapp::create_model_renderer(vk::Extent2D framebuffer_size)
{
    return new model_renderer(context.physical_device, context.device, context.descriptor_pool.get(),
                              framebuffer_size, &model_pipeline, &mdl);
}

static glm::vec3 get_trackball_position(const input_state& input, glm::vec2 mouse_position)
{
    const glm::vec2 origin(input.width / 2.f, input.height / 2.f);
    const auto radius = glm::min(input.width, input.height) / 2.f;

    const auto xy = glm::vec2(mouse_position.x, input.height - mouse_position.y - 1) - origin;

    if (dot(xy, xy) <= radius * radius / 2.f)
    {
        // Sphere
        const auto z = glm::sqrt(radius * radius - dot(xy, xy));
        return glm::vec3(xy, z);
    }
    // Hyperbola
    const auto z = (radius * radius / 2.f) / length(xy);
    return glm::vec3(xy, z);
}

void vulkanapp::update(vk::Device device, const input_state& input)
{
    vk::Result result;
    try
    {
        //suspicious: no reason to believe semaphore is unsignaled
        auto current_image = device.acquireNextImageKHR(current_swapchain.handle.get(), UINT64_MAX,
                                                        acquired_semaphore.get(),
                                                        nullptr).
                                    value;

        if (!input.ui_want_capture_mouse)
        {
            if (input.left_mouse_button_down)
            {
                // Based on https://www.khronos.org/opengl/wiki/Object_Mouse_Trackball
                const auto
                    v1 = normalize(get_trackball_position(input, input.previous_mouse_position)),
                    v2 = normalize(get_trackball_position(input, input.current_mouse_position));
                trackball_rotation = glm::quat(v1, v2) * trackball_rotation;
            }
            camera_distance *= static_cast<float>(1 - .1 * input.scroll_amount);
        }

        model_uniform_data data;
        data.projection = glm::perspective(glm::half_pi<float>(),
                                           static_cast<float>(input.width) / static_cast<float>(input.height),
                                           .001f, 100.f);
        data.model_view =
            lookAt(
                glm::vec3(0.f, 0.f, camera_distance),
                glm::vec3(0.f, 0.f, 0.f),
                glm::vec3(0.f, 1.f, 0.f)
            )
            *
            mat4_cast(trackball_rotation);

        const auto& current_frame_set = context.is_ray_tracing_supported && input.enable_ray_tracing
                                            ? ray_tracer->frame_set
                                            : default_frame_set;
        const auto& frame = current_frame_set.get(current_image);

        device.waitForFences({frame.rendered_fence.get()}, true, UINT64_MAX);
        device.resetFences({frame.rendered_fence.get()});

        frame.update(data);

        auto wait_dst_stage_mask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        context.queue.submit({
                                 vk::SubmitInfo()
                                 .setCommandBufferCount(1)
                                 .setPCommandBuffers(&frame.command_buffer.get())
                                 .setPWaitDstStageMask(&wait_dst_stage_mask)
                                 .setWaitSemaphoreCount(1)
                                 .setPWaitSemaphores(&acquired_semaphore.get())
                             }, frame.rendered_fence.get());

        result = context.queue.presentKHR(
            vk::PresentInfoKHR()
            .setSwapchainCount(1)
            .setPSwapchains(&current_swapchain.handle.get())
            .setPImageIndices(&current_image)
        );
    }
    catch (vk::OutOfDateKHRError&)
    {
        result = vk::Result::eErrorOutOfDateKHR;
    }

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
    {
        recreate_swapchain(vk::Extent2D(input.width, input.height));
    }
}

vulkanapp::~vulkanapp()
{
    context.queue.waitIdle();
}

static void error_callback(int error, const char* description)
{
    throw std::runtime_error(description);
}

static vk::UniqueSurfaceKHR create_window_surface(vk::Instance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface;
    const auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    assert(result == VK_SUCCESS);
    return vk::UniqueSurfaceKHR(surface, instance);
}

void render_to_window(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device,
                      const std::string& model_path)
{
    const auto success = glfwInit();
    assert(success);

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto* window = glfwCreateWindow(1024, 768, "Vulkan renderer", nullptr, nullptr);
    assert(window);

    auto surface = create_window_surface(instance, window);

    input_state input(window);
    auto app = vulkanapp(physical_device, device, surface.get(), vk::Extent2D(input.width, input.height), model_path);
    while (!glfwWindowShouldClose(window))
    {
        input.update();
        app.update(device, input);
    }

    glfwTerminate();

    ImGui::DestroyContext();
}
