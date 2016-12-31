#include "stdafx.h"
#include "buffer.h"
#include "pipeline.h"
#include "frame.h"
#include "dimensions.h"
#include "vulkanapp.h"

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
    fprintf(file, "%s: %s\n", pLayerPrefix, pMessage);
    fflush(file);
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
    auto props = physical_device.getQueueFamilyProperties();
    assert((props[0].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics);

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

struct swapchain_info
{
    vk::Device device;
    vk::SwapchainKHR swapchain;
    vk::CommandPool command_pool;
    std::vector<frame> frames;
    std::vector<vk::Semaphore> acquired_semaphores;
    vk::Semaphore acquired_semaphore;
    vk::Semaphore rendered_semaphore;

    void destroy()
    {
        device.destroyCommandPool(command_pool);

        for (auto frame : frames)
        {
            frame.destroy(device);
        }

        device.destroySemaphore(rendered_semaphore);
        device.destroySemaphore(acquired_semaphore);
        device.destroySwapchainKHR(swapchain);
    }
};

static swapchain_info create_swapchain(vk::PhysicalDevice physical_device, vk::Device device, vk::DescriptorPool descriptor_pool, vk::RenderPass render_pass, pipeline pipeline, vk::SurfaceKHR surface, buffer buffer)
{
    auto supported = physical_device.getSurfaceSupportKHR(0, surface);
    assert(supported);

    auto caps = physical_device.getSurfaceCapabilitiesKHR(surface);

    auto formats = physical_device.getSurfaceFormatsKHR(surface);
    assert(formats[0].format == vk::Format::eB8G8R8A8Unorm);

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
    swapchain_info.command_pool = device.createCommandPool(vk::CommandPoolCreateInfo());

    for (auto image : images)
    {
        swapchain_info.frames.push_back(frame(physical_device, device, swapchain_info.command_pool, descriptor_pool, image, render_pass, pipeline, buffer));
    }

    swapchain_info.rendered_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
    swapchain_info.acquired_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
    return swapchain_info;
}

static void update(
    vk::Device device,
    const swapchain_info& swapchain,
    vk::Queue queue)
{
    auto current_image = device.acquireNextImageKHR(swapchain.swapchain, UINT64_MAX, swapchain.acquired_semaphore, nullptr).value;
    auto frame = swapchain.frames[current_image];

    device.waitForFences({frame.rendered_fence}, true, UINT64_MAX);
    auto transform =
        glm::perspective(glm::half_pi<float>(), float(WIDTH) / float(HEIGHT), .001f, 100.f)
        *
        glm::lookAt(
            glm::vec3(0.f, 0.f, 2.f),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, -1.f, 0.f)
        );
    frame.uniform_buffer.update(device, &transform);

    device.resetFences({frame.rendered_fence});
    auto wait_dst_stage_mask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    queue.submit({
                     vk::SubmitInfo()
                     .setCommandBufferCount(1)
                     .setPCommandBuffers(&frame.command_buffer)
                     .setPWaitDstStageMask(&wait_dst_stage_mask)
                     .setWaitSemaphoreCount(1)
                     .setPWaitSemaphores(&swapchain.acquired_semaphore)
                     .setSignalSemaphoreCount(1)
                     .setPSignalSemaphores(&swapchain.rendered_semaphore)
                 }, frame.rendered_fence);

    queue.presentKHR(
        vk::PresentInfoKHR()
        .setSwapchainCount(1)
        .setPSwapchains(&swapchain.swapchain)
        .setPImageIndices(&current_image)
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&swapchain.rendered_semaphore)
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

    auto app = vulkanapp(physical_device, device, surface);

    auto swapchain = create_swapchain(physical_device, device, app.descriptor_pool, app.render_pass, app.pl, surface, app.mesh);


    while (!glfwWindowShouldClose(window))
    {
        update(device, swapchain, app.queue);
        glfwPollEvents();
    }

    app.destroy(device);

    swapchain.destroy();

    instance.destroySurfaceKHR(surface);

    glfwTerminate();

    device.destroy();
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    instance.destroy();

    fclose(file);
    return EXIT_SUCCESS;
}
