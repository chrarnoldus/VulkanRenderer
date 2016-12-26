#include "stdafx.h"

static const uint32_t WIDTH = 1024u;
static const uint32_t HEIGHT = 768u;

static PFN_vkCreateDebugReportCallbackEXT pfnCreateDebugReportCallbackEXT = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT = nullptr;

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
    std::fprintf(stderr, "%s: %s\n", pLayerPrefix, pMessage);
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
        .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
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

static vk::CommandBuffer create_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::Image image, vk::RenderPass render_pass, vk::Framebuffer framebuffer)
{
    auto command_buffer = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1)
    )[0];

    command_buffer.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));

    auto barrier = vk::ImageMemoryBarrier()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setLayerCount(VK_REMAINING_ARRAY_LAYERS)
        )
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::DependencyFlags(),
        {},
        {},
        {barrier}
    );

    auto clear_value = vk::ClearValue()
        .setColor(vk::ClearColorValue().setFloat32({0.f, 0.f, 1.f, 0.f}));

    vk::Rect2D render_area;
    render_area.extent.width = WIDTH;
    render_area.extent.height = HEIGHT;

    command_buffer.beginRenderPass(
        vk::RenderPassBeginInfo()
        .setRenderPass(render_pass)
        .setClearValueCount(1)
        .setPClearValues(&clear_value)
        .setRenderArea(render_area)
        .setFramebuffer(framebuffer)
        ,
        vk::SubpassContents::eInline
    );

    //command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    // TODO draw

    command_buffer.endRenderPass();

    command_buffer.end();

    return command_buffer;
}


int main(int argc, char** argv)
{
    auto instance = create_instance();
    auto callback = create_debug_report_callback(instance);
    auto physical_device = get_physical_device(instance);
    auto device = create_device(physical_device);
    auto queue = device.getQueue(0, 0);
    auto render_pass = create_render_pass(device);

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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    instance.destroySurfaceKHR(surface);

    glfwTerminate();

    device.destroyRenderPass(render_pass);
    device.destroy();
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    instance.destroy();

    return EXIT_SUCCESS;
}
