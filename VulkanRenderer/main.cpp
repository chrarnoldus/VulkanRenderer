#include "stdafx.h"
#include "shaders.h"

const uint32_t WIDTH = 1024u;
const uint32_t HEIGHT = 768u;

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

struct buffer_info
{
    vk::DeviceMemory memory;
    vk::Buffer buffer;

    void destroy(vk::Device device)
    {
        device.destroyBuffer(buffer);
        device.freeMemory(memory);
    }
};

static buffer_info create_buffer(vk::PhysicalDevice physical_device, vk::Device device)
{
    auto desired_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    auto props = physical_device.getMemoryProperties();

    auto memory_type_index = UINT32_MAX;
    for (auto i = uint32_t(0); i < props.memoryTypeCount; i++)
    {
        if ((props.memoryTypes[i].propertyFlags & desired_flags) == desired_flags)
        {
            memory_type_index = i;
            break;
        }
    }
    assert(memory_type_index != UINT32_MAX);

    auto queue_familiy_index = uint32_t(0);
    auto size = 6 * sizeof(vertex);

    auto memory = device.allocateMemory(
        vk::MemoryAllocateInfo()
        .setAllocationSize(size)
        .setMemoryTypeIndex(memory_type_index)
    );

    auto ptr = reinterpret_cast<vertex*>(device.mapMemory(memory, 0, VK_WHOLE_SIZE));
    ptr[0] = {-1.f, 1.f, 255, 0, 0};
    ptr[1] = {1.f, 1.f, 0, 255, 0};
    ptr[2] = {1.f, -1.f, 0, 0, 255};
    ptr[3] = {1.f, -1.f, 0, 0, 255};
    ptr[4] = {-1.f, -1.f, 0, 255, 0};
    ptr[5] = {-1.f, 1.f, 255, 0, 0};
    device.unmapMemory(memory);

    auto buffer = device.createBuffer(
        vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        .setQueueFamilyIndexCount(0)
        .setPQueueFamilyIndices(&queue_familiy_index)
        .setSharingMode(vk::SharingMode::eExclusive)
    );

    auto reqs = device.getBufferMemoryRequirements(buffer);
    assert((reqs.memoryTypeBits & 1u << memory_type_index) == 1u << memory_type_index);

    device.bindBufferMemory(buffer, memory, 0);

    buffer_info info;
    info.memory = memory;
    info.buffer = buffer;
    return info;
}

static vk::CommandBuffer create_command_buffer(vk::Device device, vk::CommandPool command_pool, vk::Image image, vk::RenderPass render_pass, vk::Pipeline pipeline, vk::Framebuffer framebuffer)
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

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    // TODO draw

    command_buffer.endRenderPass();

    command_buffer.end();

    return command_buffer;
}

struct swapchain_info
{
    vk::Device device;
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;
    std::vector<vk::Framebuffer> framebuffers;
    vk::CommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers;
    vk::Semaphore acquire_semaphore;
    vk::Semaphore render_semaphore;

    void destroy()
    {
        device.destroySemaphore(render_semaphore);
        device.destroySemaphore(acquire_semaphore);
        device.destroyCommandPool(command_pool);

        for (auto framebuffer : framebuffers)
        {
            device.destroyFramebuffer(framebuffer);
        }

        for (auto image_view : image_views)
        {
            device.destroyImageView(image_view);
        }

        device.destroySwapchainKHR(swapchain);
    }
};

static swapchain_info create_swapchain(vk::PhysicalDevice physical_device, vk::Device device, vk::RenderPass render_pass, vk::Pipeline pipeline, vk::SurfaceKHR surface)
{
    auto supported = physical_device.getSurfaceSupportKHR(0, surface);
    assert(supported);

    auto caps = physical_device.getSurfaceCapabilitiesKHR(surface);

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
    swapchain_info.images = images;
    swapchain_info.command_pool = device.createCommandPool(vk::CommandPoolCreateInfo());

    for (auto image : images)
    {
        auto view = device.createImageView(
            vk::ImageViewCreateInfo()
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setViewType(vk::ImageViewType::e2D)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setLevelCount(VK_REMAINING_MIP_LEVELS)
                .setLayerCount(1)
            )
        );
        swapchain_info.image_views.push_back(view);

        auto framebuffer = device.createFramebuffer(
            vk::FramebufferCreateInfo()
            .setRenderPass(render_pass)
            .setAttachmentCount(1)
            .setPAttachments(&view)
            .setWidth(WIDTH)
            .setHeight(HEIGHT)
            .setLayers(1)
        );
        swapchain_info.framebuffers.push_back(framebuffer);

        swapchain_info.command_buffers.push_back(create_command_buffer(
            device,
            swapchain_info.command_pool,
            image,
            render_pass,
            pipeline,
            framebuffer
        ));
    }

    swapchain_info.acquire_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
    swapchain_info.render_semaphore = device.createSemaphore(vk::SemaphoreCreateInfo());

    return swapchain_info;
}

static void update(
    vk::Device device,
    const swapchain_info& swapchain,
    vk::Queue queue)
{
    auto current_image = device.acquireNextImageKHR(swapchain.swapchain, UINT64_MAX, swapchain.acquire_semaphore, nullptr).value;

    auto wait_dst_stage_mask = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    queue.submit({
                     vk::SubmitInfo()
                     .setCommandBufferCount(1)
                     .setPCommandBuffers(&swapchain.command_buffers[current_image])
                     .setPWaitDstStageMask(&wait_dst_stage_mask)
                     .setWaitSemaphoreCount(1)
                     .setPWaitSemaphores(&swapchain.acquire_semaphore)
                     .setSignalSemaphoreCount(1)
                     .setPSignalSemaphores(&swapchain.render_semaphore)
                 }, nullptr);

    queue.presentKHR(
        vk::PresentInfoKHR()
        .setSwapchainCount(1)
        .setPSwapchains(&swapchain.swapchain)
        .setWaitSemaphoreCount(1)
        .setPImageIndices(&current_image)
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&swapchain.render_semaphore)
    );
}

int main(int argc, char** argv)
{
    auto instance = create_instance();
    auto callback = create_debug_report_callback(instance);
    auto physical_device = get_physical_device(instance);
    auto device = create_device(physical_device);
    auto queue = device.getQueue(0, 0);
    auto render_pass = create_render_pass(device);
    auto buffer = create_buffer(physical_device, device);
    auto pipeline = create_pipeline(device, render_pass, "vert.spv", "frag.spv");

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

    auto swapchain = create_swapchain(physical_device, device, render_pass, pipeline.pipeline, surface);

    while (!glfwWindowShouldClose(window))
    {
        update(device, swapchain, queue);
        glfwPollEvents();
    }

    queue.waitIdle();

    swapchain.destroy();

    instance.destroySurfaceKHR(surface);

    glfwTerminate();

    pipeline.destroy(device);
    buffer.destroy(device);
    device.destroyRenderPass(render_pass);
    device.destroy();
    pfnDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    instance.destroy();

    return EXIT_SUCCESS;
}
