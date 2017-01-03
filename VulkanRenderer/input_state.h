#pragma once
#include <glm/fwd.hpp>
#include <GLFW/glfw3.h>

struct input_state
{
    glm::vec2 last_mouse_position;
    glm::quat rotation;
    float camera_distance;
    bool left_mouse_button_down;
    bool right_mouse_button_down;

    input_state();
};

void cursor_position_callback(GLFWwindow* window, double x_position, double y_position);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double x_offset, double y_offset);