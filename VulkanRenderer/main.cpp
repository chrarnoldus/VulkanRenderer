#include "stdafx.h"
#include "buffer.h"
#include "pipeline.h"
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
        while (!glfwWindowShouldClose(window))
    {
        app.update(device);
        glfwPollEvents();
    }
    app.destroy(device);

    instance.destroySurfaceKHR(surface);

    glfwTerminate();

    device.destroy();
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    instance.destroy();

    fclose(file);
    return EXIT_SUCCESS;
}
