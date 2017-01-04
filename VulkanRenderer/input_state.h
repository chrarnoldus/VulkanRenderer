#pragma once
#include <glm/fwd.hpp>
#include <GLFW/glfw3.h>

struct input_state
{
    glm::vec2 previous_mouse_position;
    glm::vec2 current_mouse_position;
    double scroll_amount;
    double time;
    bool left_mouse_button_down;
    bool right_mouse_button_down;
    bool ui_want_capture_mouse;

    input_state(GLFWwindow* window);
    void update();
};
