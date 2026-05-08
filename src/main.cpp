#include "ECS/Engine.hpp"
#include "ECS/JobChunk.hpp"
#include "ECS/Base/ISystem.hpp"
#include "ECS/Base/Query.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/Archetype.hpp"
#include "uv.h"
using namespace ECS;

extern align_ptr<DOE> sharedEngine;

int main(int argc, char*argv[]){
    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();
    sharedEngine = make_align<DOE>();
    JobsUtility::init();
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
