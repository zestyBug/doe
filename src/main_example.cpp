/**
 * A template main file
 */
#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"
#include "uv.h"

std::unique_ptr<ECS::DOE> ECS::sharedEngine;

int main(int argc, char *argv[])
{
    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();

    ECS::sharedEngine = std::make_unique<ECS::DOE          >();
    ECS::jobsUtility  =      make_align <ECS::JobsUtility  >();

    ECS::jobsUtility->init();
    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
    uv_library_shutdown();

    ECS::sharedEngine.reset();
    ECS::jobsUtility.reset();
    ECS::TypeManager::Dispose();
    if(allocator_counter){
        printf("Memory leak count %li\n",allocator_counter);
    }
	return 0;
}