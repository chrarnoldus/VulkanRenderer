#include "stdafx.h"
#include "helpers.h"
#include "render_to_window.h"

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
    fprintf(stderr, "%s: %s\n", pLayerPrefix, pMessage);
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

vk::Instance create_instance()
{
#if _DEBUG
    const auto layerCount = 1;
    const char* layerNames[layerCount] = { "VK_LAYER_LUNARG_standard_validation" };
#else
    const auto layerCount = 0;
    const char** layerNames = nullptr;
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
    std::printf("Using physical device %s\n", props.deviceName);
    return devices[0];
}

static vk::Device create_device(vk::PhysicalDevice physical_device)
{
    auto props = physical_device.getQueueFamilyProperties();
    assert((props[0].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics);

    float priorities[] = { 0.f };
    auto queueInfo = vk::DeviceQueueCreateInfo()
        .setQueueCount(1)
        .setPQueuePriorities(priorities);

#if _DEBUG
    const auto layerCount = 1;
    const char* layerNames[layerCount] = { "VK_LAYER_LUNARG_standard_validation" };
#else
    const auto layerCount = 0;
    const char** layerNames = nullptr;
#endif

    const auto extensionCount = 1;
    const char* extensionNames[extensionCount] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    auto features = vk::PhysicalDeviceFeatures()
        .setMultiDrawIndirect(true)
        .setDrawIndirectFirstInstance(true)
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
    auto pattern = "*.ply";
    auto model_path = tinyfd_openFileDialog("Open 3D model", nullptr, 1, &pattern, nullptr, 0);
    if (!model_path)
    {
        return EXIT_FAILURE;
    }

    auto instance = create_instance();
    auto callback = create_debug_report_callback(instance);
    auto physical_device = get_physical_device(instance);
    auto device = create_device(physical_device);

    // TODO configure from command line
    if (true)
    {
        render_to_window(instance, physical_device, device, model_path);
    }
    else
    {
        render_to_image(physical_device, device, model_path, "result.png");
    }

    device.destroy();
#if _DEBUG
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
#endif
    instance.destroy();

    return EXIT_SUCCESS;
}
