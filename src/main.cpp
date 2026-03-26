#include "ECS/SystemManager.hpp"
#include "system/example.hpp"
#include "glcorearb.h"
#define _GLFW_X11 1
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"

int main(){
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
	if (!window) return 1;
    glfwMakeContextCurrent(window);
    glwInitialize(0x304);
    puts((char*)glGetString(GL_VENDOR));
    puts((char*)glGetString(GL_RENDERER));
    puts((char*)glGetString(GL_VERSION));

    glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
    {
        ECS::SystemState engine;
        engine.context = window;

        while (!glfwWindowShouldClose(window))
        {
            glClearColor(0.4f,0.1f,0,0.5);
            glClearDepth(0.0);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffer(window);
            //glfwPollEvents();
            //glfwWaitEventsTimeout(0.7);
            glfwWaitEvents();
            auto view = engine.systems.view();
            for (const auto value:view)
                if(ECS::System &s = view.get(value);s.onUpdate)
                    if(s.onUpdate(s.ctx,engine))
                        break;
        }
        {
            auto view = engine.systems.view();
            for (const auto value:view)
                if(ECS::System &s = view.get(value);s.onDestroy)
                    s.onDestroy(s.ctx,engine);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}