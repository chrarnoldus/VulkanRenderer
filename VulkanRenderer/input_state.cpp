#include "stdafx.h"
#include "input_state.h"

input_state::input_state()
    : scroll_amount(0.), time(0.), left_mouse_button_down(false), right_mouse_button_down(false), ui_wants_mouse(false)
{
}

void input_state::update()
{
    scroll_amount = 0.;
    previous_mouse_position = current_mouse_position;
    glfwPollEvents();
}

void cursor_position_callback(GLFWwindow* window, double x_position, double y_position)
{
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    input->current_mouse_position = glm::vec2(float(x_position), float(y_position));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            input->left_mouse_button_down = true;
        }
        if (action == GLFW_RELEASE)
        {
            input->left_mouse_button_down = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            input->right_mouse_button_down = true;
        }
        if (action == GLFW_RELEASE)
        {
            input->right_mouse_button_down = false;
        }
    }
}

void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    input->scroll_amount = y_offset;
}
