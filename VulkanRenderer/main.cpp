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
    std::cerr << pLayerPrefix << ": " << pMessage << std::endl;
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
    assert(devices.size() > 0);

    auto device = devices[0];
    auto props = device.getProperties();
    std::cout << "Using physical device " << props.deviceName << std::endl;
    return device;
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

cxxopts::Options parse_options(int argc, char** argv)
{
    cxxopts::Options options("VulkanRenderer", "Vulkan renderer");
    options.add_options()
        ("model", "PLY model to render. File dialog will be shown if ommitted.", cxxopts::value<std::string>(), "path")
        ("image", "PNG image to save rendering to (overwrites existing). No window will be created if specified.", cxxopts::value<std::string>(), "path")
        ("camera_position", "When using --image, specifies the camera position.", cxxopts::value<std::vector<float>>(), "x y z")
        ("camera_up", "When using --image, specifies the camera up vector.", cxxopts::value<std::vector<float>>(), "x y z")
        ("help", "Show help")
        ;

    try {
        options.parse(argc, argv);
    }
    catch (const cxxopts::OptionException& e) {
        // TODO prints garbage
        std::cerr << e.what() << std::endl;
        exit(EXIT_SUCCESS);
    }
    return options;
}

glm::vec3 std_vector_to_glm_vec3(const std::vector<float>& vector)
{
    return glm::vec3(vector[0], vector[1], vector[2]);
}

int main(int argc, char** argv)
{
    auto options = parse_options(argc, argv);

    if (options["help"].count() > 0) {
        std::cout << options.help();
        return EXIT_SUCCESS;
    }

    std::string model_path;
    auto model_path_option = options["model"];

    if (model_path_option.count() == 1)
    {
        model_path = model_path_option.as<std::string>();
    }
    else
    {
        auto pattern = "*.ply";
        auto model_path_ptr = tinyfd_openFileDialog("Open 3D model", nullptr, 1, &pattern, nullptr, 0);
        if (!model_path_ptr)
        {
            return EXIT_FAILURE;
        }
        model_path = model_path_ptr;
    }

    auto instance = create_instance();
    auto callback = create_debug_report_callback(instance);
    auto physical_device = get_physical_device(instance);
    auto device = create_device(physical_device);

    auto image_path_option = options["image"];
    if (image_path_option.count() == 1)
    {
        auto camera_position_option = options["camera_position"];
        auto camera_position = camera_position_option.count() == 3
            ? std_vector_to_glm_vec3(camera_position_option.as<std::vector<float>>())
            : glm::vec3(0.f, 0.f, 2.f);

        auto camera_up_vector = options["camera_up"];
        auto camera_up = camera_up_vector.count() == 3
            ? std_vector_to_glm_vec3(camera_up_vector.as<std::vector<float>>())
            : glm::vec3(0.f, -1.f, 0.f);

        std::cout << "Rendering to image..." << std::endl;
        render_to_image(physical_device, device, model_path, image_path_option.as<std::string>(), camera_position, camera_up);
    }
    else
    {
        std::cout << "Rendering to window..." << std::endl;
        render_to_window(instance, physical_device, device, model_path);
    }

    device.destroy();
#if _DEBUG
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
#endif
    instance.destroy();

    return EXIT_SUCCESS;
}
