#include "ECS/Engine.hpp"
#include "ECS/Base/ISystem.hpp"
#include "vulkan/VKContext.hpp"
#include "cutil/mini_test.hpp"
#include "uv.h"
#define COUNT 100
static int counter1 = 0;
static int counter2 = 0;

std::unique_ptr<ECS::Engine> ECS::sharedEngine;

struct comp_1 : ECS::IComponentData
{
    int var1 = 1;
    int var2 = 2;
    comp_1(){
    }
};
DEF_TYPE(comp_1)


struct comp_2 : ECS::ISharedComponentData
{
    int var1 = 0;
    int var2 = 0;
    comp_2() : ECS::ISharedComponentData{} {
        var1 = 0;
        var2 = 0;
    }
    comp_2(int v1,int v2) : ECS::ISharedComponentData{} {
        var1 = v1;
        var2 = v2;
    }
};
DEF_TYPE(comp_2)

struct ResourceType1 : ECS::IResource {
    int a;
    void load(void *ctx, ECS::Resource &rt){
        printf("LoaderSystem: Creating the resource...\n");
        this->a = 67;
        ECS::sharedEngine->rm.resourceReady(rt);
    }
    void free(void *ctx, ECS::Resource &rt){
        printf("LoaderSystem: destroying the resource...\n");
    }
};

ECS::Resource glob_rc;

void cb(void*,ECS::Resource rc){
    ECS::sharedEngine->rm.retain(rc);
    ResourceType1 *value = (ResourceType1*) ECS::sharedEngine->rm.getResource(rc);
    printf("cb: value = %i\n",value->a);
    glob_rc = rc;
}

TEST(Test1) {
    ECS::sharedEngine = std::make_unique<ECS::Engine>();
    //ECS::VKContext vk{};
    //loader.vk = &vk;
    //vk.initialize();
    //vk.selectDevice();
    //vk.initRender();

    //ECS::sharedEngine->am.indexBundle("./test-v2/test.zip");

    ECS::ResourceID rt = ECS::TypeManager::registerResource<ResourceType1>(nullptr);

    ECS::sharedEngine->rm.loadResource(rt, "test", nullptr, &cb);

    ECS::sharedEngine->rm.release(glob_rc);

    ECS::sharedEngine->rm.garbageCollect();

    //uv_run(uv_default_loop(),UV_RUN_DEFAULT);
}

int main(int argc, char **argv){
    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();
    mtest::run_all();
    if(uv_loop_close(loop))
        printf("Libuv error: active requests\n");
    uv_library_shutdown();
    return 0;
}
