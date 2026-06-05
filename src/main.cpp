#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"
#include "uv.h"
#include "glfw/glfw3.h"
#include "vulkan/VKContext.hpp"
using namespace ECS;

extern std::unique_ptr<DOE> sharedEngine;
extern GLFWwindow* window;

int main(int argc, char*argv[]){
    VKContext vk;
    vk.initialize();
    if (!glfwInit()) return 1;
    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();
    window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
    vk.createSurface(window);
    vk.selectDevice();
    vk.initSwapchain();
    //glwInitialize(0x304);
    sharedEngine = std::make_unique<DOE>();
    TypeManager::Initialize();
    JobsUtility::init();
    while (!glfwWindowShouldClose(window))
    {
        //glfwSwapBuffer(window);
        uv_run(loop, UV_RUN_ONCE);
        glfwPollEvents();
    }
    JobsUtility::signalQuit();
    glfwDestroyWindow(window);
    glfwTerminate();
    uv_run(loop, UV_RUN_DEFAULT);
    uv_loop_close(loop);
    uv_library_shutdown();
    sharedEngine.reset();
#ifdef DEBUG
    // one for the Threadpool jobs + 2 for TypeManager
    if(allocator_counter != 3){
        printf("Memory leak count %li\n",allocator_counter);
    }
#endif
    return 0;
}
