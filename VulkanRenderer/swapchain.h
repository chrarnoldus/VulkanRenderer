#pragma once
class swapchain
{
public:
    vk::UniqueSwapchainKHR handle;
    std::vector<vk::Image> images;

    swapchain(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface, vk::SwapchainKHR old_swapchain);
};

