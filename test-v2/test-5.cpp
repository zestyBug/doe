#include "ECS/Engine.hpp"
#include "ECS/JobChunk.hpp"
#include "ECS/Base/ISystem.hpp"
#include "ECS/Base/Query.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/Archetype.hpp"
#include "cutil/mini_test.hpp"
#include "uv.h"
using namespace ECS;

extern align_ptr<DOE> sharedEngine;

struct Position : IComponentData {
    float x;
};
DEF_TYPE(Position)
struct Speed : IComponentData {
    float x;
};
DEF_TYPE(Speed)
struct SpeedJob : IJobChunk {
    void execute(const Chunk *ch, const_span<int32_t> index){
        Archetype *arch = ch->archetype;
        uint32_t count = ch->count;
        Speed    *speedArray    = (Speed*)   arch->getComponentDataRO(ch,0,index[0]);
        Position *positionArray = (Position*)arch->getComponentDataRO(ch,0,index[1]);
        while(count--){
            positionArray->x += speedArray->x;
            ++speedArray;
            ++positionArray;
        }
    }
};
struct SpeedSystem : ISystem {
    JobChunkWrapper<SpeedJob> wrapper;
    EntityQueryImpl qd;
    uint32_t counter=0;
    SpeedSystem(DOE &e){
        Archetype *arch = e.ecs.getOrCreateArchetype(componentTypes<Entity,Speed,Position>());
        Entity entities[200];
        e.ecs.createEntities(arch,{entities,200});
        EntityQueryBuilder qb;
        qb.withAllRW(getTypeID<Position>());
        qb.withAll(getTypeID<Speed>());
        qd = e.eqm.createEntityQuery(qb);
    }
    void OnFixedUpdate(DOE &e){
        counter++;
        if(counter>100){
            JobsUtility::signalQuit();
            return;
        }
        e.scheduleQueue.push_back(Schedule{(JobChunkWrapperBase*)&wrapper,qd,false});
        //wrapper.schedule(qd,e.dpm);
    }
};

struct Test {
    static void Test1();
};
CLASS_TEST(Test,Test1){
    uv_loop_t *loop = uv_default_loop();
    align_ptr<SpeedSystem> system = make_align<SpeedSystem>(*sharedEngine);
    sharedEngine->sys.emplace_back((ISystem*)system.release());
    JobsUtility::init();
    uv_run(loop, UV_RUN_DEFAULT);
}

int main(int argc, char*argv[]){
    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();
    sharedEngine = make_align<DOE>();
    mtest::run_all();
    uv_loop_close(loop);
    uv_library_shutdown();
    sharedEngine.reset();
#ifdef DEBUG
    printf("Memory leak count %li\n",allocator_counter);
#endif
    return 0;
}
