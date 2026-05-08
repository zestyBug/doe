#include "ECS/Engine.hpp"
#include "ECS/JobChunk.hpp"
#include "ECS/Base/ISystem.hpp"
#include "ECS/Base/Query.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/Archetype.hpp"
#include "uv.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
using namespace ECS;

extern align_ptr<DOE> sharedEngine;

int main(int argc, char*argv[]){
    if (!glfwInit()) return 1;
    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();
    GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
    //glwInitialize(0x304);
    sharedEngine = make_align<DOE>();
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
    // one for the threadpool jobs
    if(allocator_counter != 1){
        printf("Memory leak count %li\n",allocator_counter);
    }
#endif
    return 0;
}
