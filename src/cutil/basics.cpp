#include "cutil/basics.hpp"
#ifdef DEBUG
#include <stdio.h>
ssize_t allocator_counter;
#endif
void* _allocate(size_t __n){
    void* ret;
    if(__n < 1)      return nullptr;
    if(__n>0x100000)  throw std::bad_alloc();
#if DOE_WIN32
    ret = (void*) _aligned_malloc(__n,ECS::Constants::CacheLineSize);
#else
    ret = (void*) aligned_alloc(ECS::Constants::CacheLineSize,__n);
#endif
    if(ret == nullptr) throw std::bad_alloc();
#ifdef DEBUG
    allocator_counter++;
    printf("allocate(): %u byte in %p\n",(uint32_t)__n,ret);
#endif
    return ret;
}
void _deallocate(void *__p){
    if(likely(__p != nullptr)){
#ifdef DEBUG
    printf("deallocate(): %p\n",__p);
    allocator_counter--;
#endif
#if DOE_WIN32
    _aligned_free(__p);
#else
    free(__p);
#endif
    }
}