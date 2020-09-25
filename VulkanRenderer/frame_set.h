#pragma once
#include "frame.h"
#include "pipeline.h"
#include "vulkan_context.h"
#include "ui_renderer.h"

class frame_set
{
    std::vector<std::unique_ptr<frame>> frames;
public:
    frame_set(std::vector<std::unique_ptr<frame>> frames);
    const frame& get(size_t current_image) const;
};


template <typename RendererFactory>
static frame_set create_frame_set(
    const vulkan_context& context,
    const std::vector<vk::Image>& images,
    RendererFactory create_model_renderer,
    const pipeline* ui_pipeline,
    const image_with_view* font_image
)
{
    std::vector<std::unique_ptr<frame>> frames;

    for (const auto& image : images)
    {
        std::vector<std::unique_ptr<renderer>> renderers;

        renderers.emplace_back(create_model_renderer());

        renderers.emplace_back(new ui_renderer(
            context.physical_device,
            context.device,
            context.descriptor_pool.get(),
            ui_pipeline,
            font_image
        ));

        frames.emplace_back(new frame(
            context.physical_device,
            context.device,
            context.command_pool.get(),
            context.descriptor_pool.get(),
            image,
            vk::Format::eB8G8R8A8Unorm,
            context.render_pass.get(),
            std::move(renderers)
        ));
    }

    return frame_set(std::move(frames));
}
