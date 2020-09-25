#include "stdafx.h"
#include "helpers.h"
#include "render_to_window.h"
#include "vulkan_context.h"

static VkBool32 debug_report_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << pCallbackData->pMessage << std::endl << std::endl;
    return false;
}

static std::optional<vk::UniqueDebugUtilsMessengerEXT> create_debug_report_callback(vk::Instance instance)
{
#if _DEBUG
    const auto info = vk::DebugUtilsMessengerCreateInfoEXT()
                      .setMessageSeverity(
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                      )
                      .setMessageType(
                          vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                      )
                      .setPfnUserCallback(debug_report_callback);

    return std::optional{instance.createDebugUtilsMessengerEXTUnique(info, nullptr)};
#else
    return std::nullopt;
#endif
}

static vk::UniqueInstance create_instance()
{
#if _DEBUG
    std::array layerNames{"VK_LAYER_KHRONOS_validation"};
#else
    std::array<const char*, 0> layerNames;
#endif

    std::array extensionNames{
#if _DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    return createInstanceUnique(
        vk::InstanceCreateInfo()
        .setPEnabledLayerNames(layerNames)
        .setPEnabledExtensionNames(extensionNames)
    );
}

static vk::PhysicalDevice get_physical_device(vk::Instance vulkan)
{
    auto devices = vulkan.enumeratePhysicalDevices();
    assert(devices.size() > 0);

    const auto device_it = std::find_if(std::begin(devices), std::end(devices), [](auto d)
    {
        return d.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    });

    const auto device = device_it != std::end(devices) ? *device_it : devices[0];
    auto props = device.getProperties();
    std::cout << "Using physical device " << props.deviceName << std::endl;
    return device;
}

static vk::UniqueDevice create_device(vk::PhysicalDevice physical_device)
{
    auto props = physical_device.getQueueFamilyProperties();
    assert((props[0].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics);

    std::array priorities{0.f};
    auto queueInfo = vk::DeviceQueueCreateInfo()
        .setQueuePriorities(priorities);

    std::vector extensionNames{
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    };

    if (is_ray_tracing_supported(physical_device))
    {
        std::cout << "Enabling ray tracing extension" << std::endl;
        extensionNames.emplace_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
    }

    auto features = vk::PhysicalDeviceFeatures()
                    .setMultiDrawIndirect(true)
                    .setDrawIndirectFirstInstance(true)
                    .setShaderClipDistance(true)
                    .setShaderCullDistance(true);

    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDevice8BitStorageFeatures,
                       vk::PhysicalDeviceFloat16Int8FeaturesKHR> device_create_info{
        vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(1)
        .setPQueueCreateInfos(&queueInfo)
        .setPEnabledExtensionNames(extensionNames)
        .setPEnabledFeatures(&features),
        vk::PhysicalDevice8BitStorageFeatures()
        .setUniformAndStorageBuffer8BitAccess(true),
        vk::PhysicalDeviceFloat16Int8FeaturesKHR()
        .setShaderInt8(true),
    };

    return physical_device.createDeviceUnique(device_create_info.get<vk::DeviceCreateInfo>());
}

glm::vec3 std_vector_to_glm_vec3(const std::vector<float>& vector)
{
    return glm::vec3(vector[0], vector[1], vector[2]);
}

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main(int argc, char** argv)
{
    try
    {
        cxxopts::Options options("VulkanRenderer", "Vulkan renderer");
        options.add_options()
            ("model", "PLY model to render. File dialog will be shown if ommitted.", cxxopts::value<std::string>(),
             "path")
            ("image", "PNG image to save rendering to (overwrites existing). No window will be created if specified.",
             cxxopts::value<std::string>(), "path")
            ("camera_position", "When using --image, specifies the camera position.",
             cxxopts::value<std::vector<float>>(), "x y z")
            ("camera_up", "When using --image, specifies the camera up vector.", cxxopts::value<std::vector<float>>(),
             "x y z")
            ("help", "Show help");

        cxxopts::ParseResult result = options.parse(argc, argv);

        if (result["help"].count() > 0)
        {
            std::cout << options.help();
            return EXIT_SUCCESS;
        }

        std::string model_path;
        auto model_path_option = result["model"];

        if (model_path_option.count() == 1)
        {
            model_path = model_path_option.as<std::string>();
        }
        else
        {
            const auto* pattern = "*.ply";
            auto* model_path_ptr = tinyfd_openFileDialog("Open 3D model", nullptr, 1, &pattern, nullptr, 0);
            if (!model_path_ptr)
            {
                return EXIT_FAILURE;
            }
            model_path = model_path_ptr;
        }

        vk::DynamicLoader dl;
        VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

        auto instance = create_instance();
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());

        auto callback = create_debug_report_callback(instance.get());
        auto physical_device = get_physical_device(instance.get());

        auto device = create_device(physical_device);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get());

        auto image_path_option = result["image"];
        if (image_path_option.count() == 1)
        {
            auto camera_position_option = result["camera_position"];
            auto camera_position = camera_position_option.count() == 3
                                       ? std_vector_to_glm_vec3(camera_position_option.as<std::vector<float>>())
                                       : glm::vec3(0.f, 0.f, 2.f);

            auto camera_up_vector = result["camera_up"];
            auto camera_up = camera_up_vector.count() == 3
                                 ? std_vector_to_glm_vec3(camera_up_vector.as<std::vector<float>>())
                                 : glm::vec3(0.f, -1.f, 0.f);

            std::cout << "Rendering to image..." << std::endl;
            render_to_image(physical_device, device.get(), model_path, image_path_option.as<std::string>(),
                            camera_position, camera_up);
        }
        else
        {
            std::cout << "Rendering to window..." << std::endl;
            render_to_window(instance.get(), physical_device, device.get(), model_path);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        throw;
    }
}
