#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"
#include "uv.h"
#include "glfw/glfw3.h"
#include "vulkan/VKContext.hpp"
using namespace ECS;

extern std::unique_ptr<DOE> sharedEngine;
extern GLFWwindow* window;
void runThread(void *arg){
    sharedEngine = std::make_unique<DOE>();
    uv_loop_t *loop = uv_default_loop();
    TypeManager::Initialize();
    JobsUtility::init();
    uv_run(loop, UV_RUN_DEFAULT);
    uv_loop_close(loop);
    sharedEngine.reset();
}
int main(int argc, char*argv[]){
    VKContext vk;
    uv_thread_t eloop;
    vk.initialize();
    if (!glfwInit()) return 1;
    uv_setup_args(argc,argv);
    window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
    vk.createSurface(window);
    vk.selectDevice();
    vk.initSwapchain();
    uv_thread_create(&eloop, &runThread, NULL);
    while (!glfwWindowShouldClose(window))
        glfwWaitEvents();
    JobsUtility::signalQuit();
    uv_thread_join(&eloop);
    glfwDestroyWindow(window);
    glfwTerminate();
    uv_library_shutdown();
#ifdef DEBUG
    // one for the Threadpool jobs + 2 for TypeManager
    if(allocator_counter){
        printf("Memory leak count %li\n",allocator_counter);
    }
#endif
    return 0;
}
