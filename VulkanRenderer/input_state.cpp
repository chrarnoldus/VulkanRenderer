#include "stdafx.h"
#include "input_state.h"

static void initialize_imgui(int width, int height)
{
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
    // TODO clipboard, ime
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto& io = ImGui::GetIO();
    if (key >= 0 && key < 512)
    {
        if (action == GLFW_PRESS)
        {
            io.KeysDown[key] = true;
        }
        if (action == GLFW_RELEASE)
        {
            io.KeysDown[key] = false;
        }
    }
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
    io.KeyAlt = (mods & GLFW_MOD_ALT) != 0;
    io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;
    io.KeySuper = (mods & GLFW_MOD_SUPER) != 0;
}

static void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    if (codepoint < UINT16_MAX)
    {
        ImGui::GetIO().AddInputCharacter(static_cast<ImWchar>(codepoint));
    }
}

static void cursor_position_callback(GLFWwindow* window, double x_position, double y_position)
{
    auto* input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);

    input->current_mouse_position = glm::vec2(static_cast<float>(x_position), static_cast<float>(y_position));
    ImGui::GetIO().MousePos = ImVec2(static_cast<float>(x_position), static_cast<float>(y_position));
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto* input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
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

    auto& io = ImGui::GetIO();
    if (button >= 0 && button < 5)
    {
        if (action == GLFW_PRESS)
        {
            io.MouseDown[button] = true;
        }
        if (action == GLFW_RELEASE)
        {
            io.MouseDown[button] = false;
        }
    }
}

static void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    auto* input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    input->scroll_amount = y_offset;
    ImGui::GetIO().MouseWheel = static_cast<float>(y_offset);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    auto* input = static_cast<input_state*>(glfwGetWindowUserPointer(window));
    assert(input);
    input->width = width;
    input->height = height;
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
}


input_state::input_state(GLFWwindow* window)
    : scroll_amount(0.), time(0.), left_mouse_button_down(false), right_mouse_button_down(false),
    ui_want_capture_mouse(false), enable_ray_tracing(false)
{
    glfwGetFramebufferSize(window, &width, &height);
    initialize_imgui(width, height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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
    const auto new_time = glfwGetTime();
    io.DeltaTime = static_cast<float>(new_time - time);
    time = new_time;
    ui_want_capture_mouse = io.WantCaptureMouse;

    ImGui::NewFrame();
    ImGui::Checkbox("Enable ray tracing", &enable_ray_tracing);
    ImGui::Render();
}
