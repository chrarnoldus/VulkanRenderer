#include "stdafx.h"
#include "swapchain.h"

static vk::UniqueSwapchainKHR create_swapchain(vk::PhysicalDevice physical_device, vk::Device device,
                                               vk::SurfaceKHR surface, vk::SwapchainKHR old_swapchain)
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
        .setOldSwapchain(old_swapchain)
    );
}

swapchain::swapchain(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, vk::SwapchainKHR old_swapchain)
    : handle(create_swapchain(physical_device, device, surface, old_swapchain))
    , images(device.getSwapchainImagesKHR(handle.get()))
{
}
