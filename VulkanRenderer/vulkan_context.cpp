#include "stdafx.h"
#include "vulkan_context.h"
#include "helpers.h"

bool is_ray_tracing_supported(vk::PhysicalDevice physical_device)
{
    auto extensions = physical_device.enumerateDeviceExtensionProperties();
    return std::ranges::any_of(extensions, [](auto& e)
        {
            return strcmp(e.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0;
        });
}

vulkan_context::vulkan_context(vk::PhysicalDevice physical_device, vk::Device device)
    : physical_device(physical_device)
    , device(device)
    , queue(device.getQueue(0, 0))
    , command_pool(device.createCommandPoolUnique(vk::CommandPoolCreateInfo()))
    , render_pass(create_render_pass(device, vk::Format::eB8G8R8A8Unorm, vk::ImageLayout::ePresentSrcKHR))
    , descriptor_pool(create_descriptor_pool(device))
    , is_ray_tracing_supported(::is_ray_tracing_supported(physical_device))
{
}
