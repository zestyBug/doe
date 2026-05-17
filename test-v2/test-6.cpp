#include "cutil/mini_test.hpp"
#include "ECS/AssetManager.hpp"
#include "uv.h"
using namespace ECS;
align_ptr<int> x{nullptr};
uint32_t allocator_counter2 = 0;
void* malloc_func(size_t size)
{
    uint8_t *ptr = (uint8_t *)malloc(size+Constants::CacheLineSize);
    #ifdef DEBUG
        allocator_counter2++;
    #endif
    *(size_t*)ptr = size;
    ptr+=Constants::CacheLineSize;
    printf("malloc_func(%lu);// %p\n",size,ptr);
    return ptr;
}
void* realloc_func(void* ptr, size_t size)
{
    uint8_t *nptr = (uint8_t *)malloc(size+Constants::CacheLineSize);
    #ifdef DEBUG
        allocator_counter2++;
    #endif
    *(size_t*)nptr = size;
    nptr+=Constants::CacheLineSize;
    uint8_t *pptr = (uint8_t*)ptr - Constants::CacheLineSize;
    printf("realloc_func(%p(%lu),%lu);// %p\n",ptr,ptr?*(size_t*)pptr:0,size,nptr);
    if(ptr) {
        memcpy(nptr, ptr,  std::min(*(size_t*)pptr,size));
        free(pptr);
        #ifdef DEBUG
            allocator_counter2--;
        #endif
    }
    return nptr;
}
void* calloc_func(size_t count, size_t size)
{
    size *= count;
    uint8_t *ptr = (uint8_t *)malloc(size+Constants::CacheLineSize);
    #ifdef DEBUG
        allocator_counter2++;
    #endif
    *(size_t*)ptr = size;
    ptr+=Constants::CacheLineSize;
    printf("calloc_func(%lu);// %p\n",size,ptr);
    memset(ptr,0,size);
    return ptr;
}
void free_func(void* ptr)
{
    if(ptr){
        uint8_t *sptr = (uint8_t*)ptr - Constants::CacheLineSize;
        printf("free_func(%p);// %lu\n", ptr, *(size_t*)sptr);
        free(sptr);
        #ifdef DEBUG
            allocator_counter2--;
        #endif
    }
}

static void onCB(std::unique_ptr<uint8_t[]> ptr,uint32_t size, uint32_t offset){
    uint8_t *data = ptr.get() + offset;
}

int main(int argc, char*argv[]){
    uv_replace_allocator(
        &malloc_func,
        &realloc_func,
        &calloc_func,
        &free_func
    );
    {
        uv_setup_args(argc,argv);
        uv_loop_t *loop = uv_default_loop();
        AssetsManager am;
        am.indexBundle("./dump/New folder.zip");
        am.open("./dump/New folder.zip","docs/code/cgi/main.c",&onCB);
        uv_run(loop,UV_RUN_DEFAULT);
        uv_loop_close(loop);
        uv_library_shutdown();
    }
#ifdef DEBUG
    // one for the threadpool
    printf("Memory leak count %li\n",allocator_counter);
    printf("Memory leak count %li\n",allocator_counter2);
#endif
    return 0;
}
