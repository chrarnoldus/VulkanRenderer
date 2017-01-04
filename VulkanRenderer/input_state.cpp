#include "stdafx.h"
#include "input_state.h"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
    {
        io.KeysDown[key] = true;
    }
    if (action == GLFW_RELEASE)
    {
        io.KeysDown[key] = false;
    }
    io.KeyCtrl = bool(mods & GLFW_MOD_CONTROL);
    io.KeyAlt = bool(mods & GLFW_MOD_ALT);
    io.KeyShift = bool(mods & GLFW_MOD_SHIFT);
    io.KeySuper = bool(mods & GLFW_MOD_SUPER);
}

static void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    if (codepoint < UINT16_MAX)
    {
        ImGui::GetIO().AddInputCharacter(ImWchar(codepoint));
    }
}

static void cursor_position_callback(GLFWwindow* window, double x_position, double y_position)
{
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);

    input->current_mouse_position = glm::vec2(float(x_position), float(y_position));
    ImGui::GetIO().MousePos = ImVec2(float(x_position), float(y_position));
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    auto& io = ImGui::GetIO();

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            input->left_mouse_button_down = true;
            io.MouseDown[0] = true;
        }
        if (action == GLFW_RELEASE)
        {
            input->left_mouse_button_down = false;
            io.MouseDown[0] = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            input->right_mouse_button_down = true;
            io.MouseDown[1] = true;
        }
        if (action == GLFW_RELEASE)
        {
            input->right_mouse_button_down = false;
            io.MouseDown[1] = false;
        }
    }
}

static void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    auto input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    input->scroll_amount = y_offset;
    ImGui::GetIO().MouseWheel = y_offset;
}

input_state::input_state(GLFWwindow* window)
    : scroll_amount(0.), time(0.), left_mouse_button_down(false), right_mouse_button_down(false), ui_want_capture_mouse(false)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    time = glfwGetTime();
}

void input_state::update()
{
    scroll_amount = 0.;
    previous_mouse_position = current_mouse_position;
    glfwPollEvents();

    auto& io = ImGui::GetIO();
    auto new_time = glfwGetTime();
    io.DeltaTime = float(new_time - time);
    time = new_time;

    ImGui::NewFrame();
    ui_want_capture_mouse = io.WantCaptureMouse;
}
