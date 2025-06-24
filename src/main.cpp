#include "ECS/SystemManager.hpp"
#include "glfw/glfw3.h"
#include <stdio.h>

void error_callback(int error, const char* description)
{
    printf("Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    int i;
    for (i = 0;  i < count;  i++)
        printf("%s\n",paths[i]);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        puts("pop menu!");
}

void character_callback(GLFWwindow* window, unsigned int codepoint)
{
}

int main()
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
    if (!window)
        return 1;

    glfwSetDropCallback(window, drop_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCharCallback(window, character_callback);

    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    {
        ECS::SystemState engine;
        while (!glfwWindowShouldClose(window))
        {
            //glfwPollEvents();
            //glfwWaitEventsTimeout(0.7);
            glfwWaitEvents();
            auto view = engine.manager.systems.view();
            for (const auto value:view)
                if(ECS::System *s = view.get(value).get();s != nullptr && s->onUpdate != nullptr)
                    s->onUpdate(s,engine);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
