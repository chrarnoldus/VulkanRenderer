#include "stdafx.h"

static const uint32_t WIDTH = 1024u;
static const uint32_t HEIGHT = 768u;

static void error_callback(int error, const char* description)
{
    throw std::runtime_error(description);
}

int main(int argc, char** argv)
{
    auto success = glfwInit();
    assert(success);

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan renderer", nullptr, nullptr);
    assert(window);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}
