#include "stdafx.h"
#include "shaders.h"
#include "buffer.h"

const uint32_t WIDTH = 1024u;
const uint32_t HEIGHT = 768u;

static PFN_vkCreateDebugReportCallbackEXT pfnCreateDebugReportCallbackEXT = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT = nullptr;

static FILE* file = nullptr;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData)
{
    std::fprintf(file, "%s: %s\n", pLayerPrefix, pMessage);
    return false;
}

static VkDebugReportCallbackEXT create_debug_report_callback(VkInstance vulkan)
{
    pfnCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(vulkan, "vkCreateDebugReportCallbackEXT")
    );
    pfnDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(vulkan, "vkDestroyDebugReportCallbackEXT")
    );

#if _DEBUG
    VkDebugReportCallbackCreateInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    info.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    info.pfnCallback = &debug_report_callback;

    VkDebugReportCallbackEXT callback;
    auto error = pfnCreateDebugReportCallbackEXT(vulkan, &info, nullptr, &callback);
    assert(!error);
    return callback;
#else
    return nullptr;
#endif
}

static void error_callback(int error, const char* description)
{
    throw std::runtime_error(description);
}

vk::Instance create_instance()
{
#if _DEBUG
    const auto layerCount = 1;
    const char* layerNames[layerCount] = {"VK_LAYER_LUNARG_standard_validation"};
#else
    const auto layerCount = 0;
    const char **layerNames = nullptr;
#endif

    const auto extensionCount = 3;
    const char* extensionNames[extensionCount] = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    return vk::createInstance(
        vk::InstanceCreateInfo()
        .setEnabledLayerCount(layerCount)
        .setPpEnabledLayerNames(layerNames)
        .setEnabledExtensionCount(extensionCount)
        .setPpEnabledExtensionNames(extensionNames)
    );
}

static vk::PhysicalDevice get_physical_device(vk::Instance vulkan)
{
    auto devices = vulkan.enumeratePhysicalDevices();
    assert(devices.size() == 1);

    auto props = devices[0].getProperties();
    std::printf("Using physical device: %s\n", props.deviceName);
    return devices[0];
}

static vk::Device create_device(vk::PhysicalDevice physical_device)
{
    float priorities[] = {0.f};
    auto queueInfo = vk::DeviceQueueCreateInfo()
        .setQueueCount(1)
        .setPQueuePriorities(priorities);

#if _DEBUG
    const auto layerCount = 1;
    const char* layerNames[layerCount] = {"VK_LAYER_LUNARG_standard_validation"};
#else
    const auto layerCount = 0;
    const char **layerNames = nullptr;
#endif

    const auto extensionCount = 1;
    const char* extensionNames[extensionCount] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    auto features = vk::PhysicalDeviceFeatures()
        .setShaderClipDistance(true)
        .setShaderCullDistance(true);

    return physical_device.createDevice(
        vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(1)
        .setPQueueCreateInfos(&queueInfo)
        .setEnabledLayerCount(layerCount)
        .setPpEnabledLayerNames(layerNames)
        .setEnabledExtensionCount(1)
        .setPpEnabledExtensionNames(extensionNames)
        .setPEnabledFeatures(&features)
    );
}

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

static buffer create_uniform_buffer(vk::PhysicalDevice physical_device, vk::Device device)
{
    auto transform =
        glm::perspective(glm::half_pi<float>(), float(WIDTH) / float(HEIGHT), .001f, 100.f)
        *
        glm::lookAt(
            glm::vec3(0.f, 0.f, 2.f),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, -1.f, 0.f)
        );

    auto size = sizeof(transform);
    auto buf = buffer(physical_device, device, vk::BufferUsageFlagBits::eUniformBuffer, size);
    buf.update(device, &transform);
    return buf;
}

static vk::CommandBuffer create_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::RenderPass render_pass, shader_info pipeline, vk::Framebuffer framebuffer, buffer buf)
{
    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));

    auto clear_value = vk::ClearValue()
        .setColor(vk::ClearColorValue().setFloat32({0.f, 1.f, 1.f, 1.f}));

    vk::Rect2D render_area;
    render_area.extent.width = WIDTH;
    render_area.extent.height = HEIGHT;

    command_buffer.beginRenderPass(
        vk::RenderPassBeginInfo()
        .setRenderPass(render_pass)
        .setClearValueCount(1)
        .setPClearValues(&clear_value)
        .setRenderArea(render_area)
        .setFramebuffer(framebuffer),
        vk::SubpassContents::eInline
    );

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, pipeline.descriptor_sets, {});

    command_buffer.bindVertexBuffers(0, {buf.buf}, {0});

    command_buffer.draw(6, 1, 0, 0);

    command_buffer.endRenderPass();

    command_buffer.end();

    return command_buffer;
}

struct swapchain_info
{
    vk::Device device;
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;
    std::vector<vk::Framebuffer> framebuffers;
    vk::CommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers;
    vk::Semaphore acquire_semaphore;
    vk::Semaphore render_semaphore;

    void destroy()
    {
        device.destroySemaphore(render_semaphore);
        device.destroySemaphore(acquire_semaphore);
        device.destroyCommandPool(command_pool);

        for (auto framebuffer : framebuffers)
        {
            device.destroyFramebuffer(framebuffer);
        }

        for (auto image_view : image_views)
        {
            device.destroyImageView(image_view);
        }

        device.destroySwapchainKHR(swapchain);
    }
};

static swapchain_info create_swapchain(vk::PhysicalDevice physical_device, vk::Device device, vk::RenderPass render_pass, shader_info pipeline, vk::SurfaceKHR surface, buffer buffer)
{
    auto supported = physical_device.getSurfaceSupportKHR(0, surface);
    assert(supported);

    auto caps = physical_device.getSurfaceCapabilitiesKHR(surface);

    auto swapchain = device.createSwapchainKHR(
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

    swapchain_info swapchain_info = {};
    swapchain_info.device = device;
    swapchain_info.swapchain = swapchain;
    swapchain_info.images = images;
    swapchain_info.command_pool = device.createCommandPool(vk::CommandPoolCreateInfo());

    for (auto image : images)
    {
        auto view = device.createImageView(
            vk::ImageViewCreateInfo()
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setViewType(vk::ImageViewType::e2D)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setLevelCount(VK_REMAINING_MIP_LEVELS)
                .setLayerCount(1)
            )
        );
        swapchain_info.image_views.push_back(view);

        auto framebuffer = device.createFramebuffer(
            vk::FramebufferCreateInfo()
            .setRenderPass(render_pass)
            .setAttachmentCount(1)
            .setPAttachments(&view)
            .setWidth(WIDTH)
            .setHeight(HEIGHT)
            .setLayers(1)
        );
        swapchain_info.framebuffers.push_back(framebuffer);

        swapchain_info.command_buffers.push_back(create_command_buffer(
            device,
            swapchain_info.command_pool,
            render_pass,
            pipeline,
            framebuffer,
            buffer
        ));
    }

    swapchain_info.acquire_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
    swapchain_info.render_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());

    return swapchain_info;
}

static void update(
    vk::Device device,
    const swapchain_info& swapchain,
    vk::Queue queue)
{
    auto current_image = device.acquireNextImageKHR(swapchain.swapchain, UINT64_MAX, swapchain.acquire_semaphore, nullptr).value;

    auto wait_dst_stage_mask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    queue.submit({
                     vk::SubmitInfo()
                     .setCommandBufferCount(1)
                     .setPCommandBuffers(&swapchain.command_buffers[current_image])
                     .setPWaitDstStageMask(&wait_dst_stage_mask)
                     .setWaitSemaphoreCount(1)
                     .setPWaitSemaphores(&swapchain.acquire_semaphore)
                     .setSignalSemaphoreCount(1)
                     .setPSignalSemaphores(&swapchain.render_semaphore)
                 }, nullptr);

    queue.presentKHR(
        vk::PresentInfoKHR()
        .setSwapchainCount(1)
        .setPSwapchains(&swapchain.swapchain)
        .setPImageIndices(&current_image)
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&swapchain.render_semaphore)
    );
}

int main(int argc, char** argv)
{
#if _DEBUG
    auto file_ok = fopen_s(&file, "debug_report.log", "w");
    assert(file_ok == 0);
#endif

    auto instance = create_instance();
    auto callback = create_debug_report_callback(instance);
    auto physical_device = get_physical_device(instance);
    auto device = create_device(physical_device);
    auto queue = device.getQueue(0, 0);
    auto render_pass = create_render_pass(device);
    auto mesh = create_mesh(physical_device, device);
    auto uniform_buffer = create_uniform_buffer(physical_device, device);
    auto pipeline = create_pipeline(device, render_pass, uniform_buffer, "vert.spv", "frag.spv");

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

    auto swapchain = create_swapchain(physical_device, device, render_pass, pipeline, surface, mesh);

    while (!glfwWindowShouldClose(window))
    {
        update(device, swapchain, queue);
        glfwPollEvents();
    }

    queue.waitIdle();

    swapchain.destroy();

    instance.destroySurfaceKHR(surface);

    glfwTerminate();

    pipeline.destroy(device);
    uniform_buffer.destroy(device);
    mesh.destroy(device);
    device.destroyRenderPass(render_pass);
    device.destroy();
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    instance.destroy();

    return EXIT_SUCCESS;
}
