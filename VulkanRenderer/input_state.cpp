#include "stdafx.h"
#include "input_state.h"
#include "dimensions.h"

input_state::input_state()
    : camera_distance(2.f), left_mouse_button_down(false), right_mouse_button_down(false)
{
}

static glm::vec3 get_trackball_position(glm::vec2 mouse_position)
{
    glm::vec2 origin(WIDTH / 2.f, HEIGHT / 2.f);
    auto radius = glm::min(WIDTH, HEIGHT) / 2.f;

    auto xy = glm::vec2(mouse_position.x, HEIGHT - mouse_position.y - 1) - origin;

    if (glm::dot(xy, xy) <= radius * radius / 2.f)
    {
        // Sphere
        auto z = glm::sqrt(radius * radius - glm::dot(xy, xy));
        return glm::vec3(xy, z);
    }
    else
    {
        // Hyperbola
        auto z = (radius * radius / 2.f) / glm::length(xy);
        return glm::vec3(xy, z);
    }
}


void cursor_position_callback(GLFWwindow* window, double x_position, double y_position)
{
    auto mouse_position = glm::vec2(float(x_position), float(y_position));
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    if (input->left_mouse_button_down)
    {
        auto
            v1 = glm::normalize(get_trackball_position(input->last_mouse_position)),
            v2 = glm::normalize(get_trackball_position(mouse_position));
        input->rotation = glm::quat(v1, v2) * input->rotation;
    }
    input->last_mouse_position = mouse_position;
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
    input->camera_distance *= float(1 - .1 * y_offset);
}
