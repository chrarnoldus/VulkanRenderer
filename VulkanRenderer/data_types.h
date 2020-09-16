#pragma once
#include <glm/fwd.hpp>
#include <imgui.h>

struct vertex
{
    glm::i16vec3 position;
    glm::i16vec3 normal;
    glm::u8vec3 color;
};

struct model_uniform_data
{
    glm::mat4 projection;
    glm::mat4 model_view;
};

#define MAX_UI_DRAW_COUNT 64 // must be kept in sync with shader

struct ui_uniform_data
{
    float screen_width;
    float screen_height;
    float padding[2];
    ImVec4 clip_rects[MAX_UI_DRAW_COUNT];
};
