#include "ECS/Engine.hpp"
#include "ECS/JobChunk.hpp"
#include "ECS/Base/ISystem.hpp"
#include "ECS/Base/Query.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/Archetype.hpp"
#include "cutil/mini_test.hpp"
#include "uv.h"
using namespace ECS;

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
    SpeedSystem(DOE*e){
        EntityQueryBuilder qb;
        qb.withAllRW(getTypeID<Position>());
        qb.withAll(getTypeID<Speed>());
        qd = e->eqm.createEntityQuery(qb);
    }
    void OnUpdate(DOE*e){
        wrapper.schedule(qd,e->dpm);
    }
};

struct Test {
    static void Test1();
};
CLASS_TEST(Test,Test1){
    uv_loop_t *loop = uv_default_loop();
    JobsUtility.INIT(2);
    align_ptr<DOE> e = make_align<DOE>();
    align_ptr<SpeedSystem> system = make_align<SpeedSystem>(e.get());
    e->sys.emplace_back((ISystem*)system.release());
    Archetype *arch = e->ecs.getOrCreateArchetype(componentTypes<Entity,Speed,Position>());
    Entity entities[200];
    e->ecs.createEntities(arch,{entities,200});
    uint32_t counter=100000;
    do {
        e->eqm.updateNewArchetypes();
        e->dpm.clear();

        for(uint32_t i=0;i<e->sys.size();i++)
            e->sys[i]->OnUpdate(e.get());

        JobsUtility.prepareJobs();
        JobsUtility.signalStart();
        while(!JobsUtility.isFinished())
            std::this_thread::yield();
        uv_run(loop, UV_RUN_DEFAULT);
        JobsUtility.reset();
    } while(--counter);
    uv_loop_close(loop);
}

int main(){
    mtest::run_all();
#ifdef DEBUG
    if(allocator_counter){
        printf("Memory leak count %li\n",allocator_counter);
    }
#endif
    return 0;
}
