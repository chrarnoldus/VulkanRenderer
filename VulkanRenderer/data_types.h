#pragma once
#include <glm/fwd.hpp>
#include <imgui.h>

struct a2b10g10r10_snorm_pack32
{
    a2b10g10r10_snorm_pack32(glm::vec3 rgb);

    signed int r : 10;
    signed int g : 10;
    signed int b : 10;
    signed int a : 2;
};

struct a2b10g10r10_unorm_pack32
{
    a2b10g10r10_unorm_pack32(glm::vec3 rgb);

    unsigned int r : 10;
    unsigned int g : 10;
    unsigned int b : 10;
    unsigned int a : 2;
};

struct vertex
{
    glm::i16vec3 position;
    a2b10g10r10_snorm_pack32 normal;
    a2b10g10r10_unorm_pack32 color;
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
