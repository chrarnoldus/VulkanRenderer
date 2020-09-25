#include "stdafx.h"
#include "render_to_window.h"
#include "dimensions.h"
#include "vulkan_context.h"
#include "input_state.h"
#include "model.h"
#include "pipeline.h"
#include "frame.h"
#include "frame_set.h"
#include "model_renderer.h"
#include "ray_tracing_model.h"
#include "ray_tracing_renderer.h"
#include "ui_renderer.h"

class ray_tracer
{
public:
    ray_tracing_model ray_tracing_model;
    pipeline textured_quad_pipeline;
    pipeline model_pipeline;
    std::unique_ptr<buffer> shader_binding_table;
    frame_set frame_set;
    ray_tracer(
        const vulkan_context& context,
        const std::vector<vk::Image>& images,
        const model* model,
        const pipeline* ui_pipeline,
        const image_with_view* font_image);
};

template <typename RendererFactory>
static frame_set create_frame_set(
    const vulkan_context& context,
    const std::vector<vk::Image>& images,
    RendererFactory create_model_renderer,
    const pipeline* ui_pipeline,
    const image_with_view* font_image
)
{
    std::vector<std::unique_ptr<frame>> frames;

    for (const auto& image : images)
    {
        std::vector<std::unique_ptr<renderer>> renderers;

        renderers.emplace_back(create_model_renderer());

        renderers.emplace_back(new ui_renderer(
            context.physical_device,
            context.device,
            context.descriptor_pool.get(),
            ui_pipeline,
            font_image
        ));

        frames.emplace_back(new frame(
            context.physical_device,
            context.device,
            context.command_pool.get(),
            context.descriptor_pool.get(),
            image,
            vk::Format::eB8G8R8A8Unorm,
            context.render_pass.get(),
            std::move(renderers)
        ));
    }

    return frame_set(std::move(frames));
}

ray_tracer::ray_tracer(
    const vulkan_context& context,
    const std::vector<vk::Image>& images,
    const model* model,
    const pipeline* ui_pipeline,
    const image_with_view* font_image)
    : ray_tracing_model(context.physical_device, context.device, context.command_pool.get(), context.queue, model)
      , textured_quad_pipeline(create_textured_quad_pipeline(context.device, context.render_pass.get()))
      , model_pipeline(create_ray_tracing_pipeline(context.device))
      , shader_binding_table(
          create_shader_binding_table(context.physical_device, context.device, model_pipeline.pl.get()))
      , frame_set(create_frame_set(context, images, [&]()
      {
          return new ray_tracing_renderer(context.physical_device, context.device, context.descriptor_pool.get(),
                                          &model_pipeline, &textured_quad_pipeline,
                                          shader_binding_table.get(), &ray_tracing_model);
      }, ui_pipeline, font_image))

{
}

class vulkanapp
{
    vulkan_context context;
    model mdl;
    pipeline textured_quad_pipeline;
    pipeline model_pipeline;
    pipeline ui_pipeline;
    vk::UniqueSemaphore acquired_semaphore;
    vk::UniqueSwapchainKHR swapchain;
    image_with_view font_image;
    std::vector<vk::Image> images;
    std::unique_ptr<ray_tracer> ray_tracer;
    frame_set default_frame_set;
    glm::quat trackball_rotation;
    float camera_distance;

public:
    vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface,
              const std::string& model_path);
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

static vk::UniqueSwapchainKHR create_swapchain(vk::PhysicalDevice physical_device, vk::Device device,
                                               vk::SurfaceKHR surface)
{
    const auto supported = physical_device.getSurfaceSupportKHR(0, surface);
    assert(supported);

    const auto caps = physical_device.getSurfaceCapabilitiesKHR(surface);

    auto formats = physical_device.getSurfaceFormatsKHR(surface);
    assert(formats[0].format == vk::Format::eB8G8R8A8Unorm);

    return device.createSwapchainKHRUnique(
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
}

vulkanapp::vulkanapp(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface,
                     const std::string& model_path)
    : context(physical_device, device)
      , mdl(read_model(physical_device, device, context.command_pool.get(), context.queue, model_path))
      , textured_quad_pipeline(create_textured_quad_pipeline(device, context.render_pass.get()))
      , model_pipeline(create_model_pipeline(device, context.render_pass.get()))
      , ui_pipeline(create_ui_pipeline(device, context.render_pass.get()))
      , acquired_semaphore(device.createSemaphoreUnique(vk::SemaphoreCreateInfo()))
      , swapchain(create_swapchain(physical_device, device, surface))
      , font_image(load_font_image(physical_device, device, context.command_pool.get(), context.queue))
      , images(device.getSwapchainImagesKHR(swapchain.get()))
      , ray_tracer(context.is_ray_tracing_supported ? std::make_unique<class ray_tracer>(context, images, &mdl, &ui_pipeline, &font_image) : nullptr)
      , default_frame_set(create_frame_set(context, images, [&]()
        {
            return new model_renderer(physical_device, device, context.descriptor_pool.get(), &model_pipeline, &mdl);
        }, &ui_pipeline, &font_image))
      , camera_distance(2.f)
{
}

static glm::vec3 get_trackball_position(glm::vec2 mouse_position)
{
    const glm::vec2 origin(WIDTH / 2.f, HEIGHT / 2.f);
    const auto radius = glm::min(WIDTH, HEIGHT) / 2.f;

    const auto xy = glm::vec2(mouse_position.x, HEIGHT - mouse_position.y - 1) - origin;

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
    //suspicious: no reason to believe semaphore is unsignaled
    auto current_image = device.acquireNextImageKHR(swapchain.get(), UINT64_MAX, acquired_semaphore.get(), nullptr).
                                value;

    if (!input.ui_want_capture_mouse)
    {
        if (input.left_mouse_button_down)
        {
            // Based on https://www.khronos.org/opengl/wiki/Object_Mouse_Trackball
            const auto
                v1 = normalize(get_trackball_position(input.previous_mouse_position)),
                v2 = normalize(get_trackball_position(input.current_mouse_position));
            trackball_rotation = glm::quat(v1, v2) * trackball_rotation;
        }
        camera_distance *= static_cast<float>(1 - .1 * input.scroll_amount);
    }

    model_uniform_data data;
    data.projection = glm::perspective(glm::half_pi<float>(), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT),
                                       .001f, 100.f);
    data.model_view =
        lookAt(
            glm::vec3(0.f, 0.f, camera_distance),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, 1.f, 0.f)
        )
        *
        mat4_cast(trackball_rotation);

    const auto& current_frame_set = context.is_ray_tracing_supported && input.enable_ray_tracing ? ray_tracer->frame_set : default_frame_set;
    const auto& frame = current_frame_set.get(current_image);

    device.waitForFences({frame.rendered_fence.get()}, true, UINT64_MAX);
    device.resetFences({frame.rendered_fence.get()});

    frame.update(data);

    auto wait_dst_stage_mask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    context.queue.submit({
                             vk::SubmitInfo()
                             .setCommandBufferCount(1)
                             .setPCommandBuffers(&frame.command_buffer)
                             .setPWaitDstStageMask(&wait_dst_stage_mask)
                             .setWaitSemaphoreCount(1)
                             .setPWaitSemaphores(&acquired_semaphore.get())
                         }, frame.rendered_fence.get());

    context.queue.presentKHR(
        vk::PresentInfoKHR()
        .setSwapchainCount(1)
        .setPSwapchains(&swapchain.get())
        .setPImageIndices(&current_image)
    );
}

vulkanapp::~vulkanapp()
{
    context.queue.waitIdle();
}

static void initialize_imgui()
{
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(WIDTH), static_cast<float>(HEIGHT));
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
    initialize_imgui();

    const auto success = glfwInit();
    assert(success);

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan renderer", nullptr, nullptr);
    assert(window);

    auto surface = create_window_surface(instance, window);

    input_state input(window);
    auto app = vulkanapp(physical_device, device, surface.get(), model_path);
    while (!glfwWindowShouldClose(window))
    {
        input.update();
        app.update(device, input);
    }

    glfwTerminate();

    ImGui::DestroyContext();
}
