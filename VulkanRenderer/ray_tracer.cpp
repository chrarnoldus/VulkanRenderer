#include "stdafx.h"
#include "ray_tracer.h"
#include "ray_tracing_renderer.h"

ray_tracer::ray_tracer(
    const vulkan_context& context,
    const std::vector<vk::Image>& images,
    const model* model,
    const pipeline* ui_pipeline,
    const image_with_view* font_image)
    : ray_tracing_model(context.physical_device, context.device, context.command_pool.get(), context.queue, model)
      , textured_quad_pipeline(create_textured_quad_pipeline(context.device, context.render_pass.get()))
      , model_pipeline(create_ray_tracing_pipeline(context.device))
      , shader_binding_table(
          create_shader_binding_table(context.physical_device, context.device, model_pipeline.pl.get()))
      , frame_set(create_frame_set(context, images, [&]()
      {
          return new ray_tracing_renderer(context.physical_device, context.device, context.descriptor_pool.get(),
                                          &model_pipeline, &textured_quad_pipeline,
                                          shader_binding_table.get(), &ray_tracing_model);
      }, ui_pipeline, font_image))

{
}
