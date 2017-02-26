#pragma once
#include <vulkan/vulkan.hpp>

void render_to_window(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device, const std::string& model_path);
