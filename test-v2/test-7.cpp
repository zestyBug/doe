#include "cutil/mini_test.hpp"
#include "ECS/AssetsManager.hpp"
#include "ECS/ResourceManager.hpp"
#include "uv.h"
using namespace ECS;

uint32_t allocator_counter2 = 0;
AssetsManager *_am;
ResourceManager *_rm;
Resource _rc;
void *ptr;

void cb(void *ctx,align_ptr<uint8_t[]> value, uint32_t size){
    ptr = value.get();
    ResourceTask *rt = (ResourceTask*)ctx;
    *(uint8_t**)rt->value = value.release();
    if(size)
        rt->state = ResourceTask::ResourceState::Loaded;
    else
        rt->state = ResourceTask::ResourceState::Failed;
    for(ResourceTask::Waiter &w : rt->waiters)
        w.cb(w.ctx, rt->res);
    rt->waiters.clear();
    _rm->refCountDecrease(rt->res);
}
void Load(void*,ResourceTask *rt){
    _am->open("./dump/test.zip", "test.c", rt, &cb);
}
void Free(void*,ResourceTask &rt){
    uint8_t *addr = *(uint8_t**)rt.value;
    if(addr != ptr)
        throw std::runtime_error("UNEXPECTED");
    if(_rc.index != rt.res.index)
        throw std::runtime_error("UNEXPECTED");
    allocator().deallocate(*(uint8_t**)rt.value);
}
void Waiter(void*,Resource rc){
    _rm->refCountIncrease(rc);
    _rc = rc;
}

int main(int argc, char*argv[])
{
    {
        uv_setup_args(argc,argv);
        uv_loop_t *loop = uv_default_loop();
        AssetsManager am;
        _am=&am;
        am.indexBundle("./dump/test.zip");
        ResourceManager rm;
        _rm=&rm;
        ResourceType id = rm.registerResourceType( &Load, &Free, &rm, 20);
        rm.loadResource(id, "test", nullptr, &Waiter);
        uv_run(loop,UV_RUN_DEFAULT);
        void *value = rm.getResource(_rc);
        rm.refCountDecrease(_rc);
        if(uv_loop_close(loop))
            printf("Libuv error: active requests\n");
        uv_library_shutdown();
    }
#ifdef DEBUG
    // one for the threadpool
    printf("Memory leak count %li\n",allocator_counter);
    printf("Memory leak count %li\n",allocator_counter2);
#endif
    return 0;
}
