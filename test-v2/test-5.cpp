#include "ECS/Engine.hpp"
#include "ECS/JobChunk.hpp"
#include "ECS/Base/ISystem.hpp"
#include "cutil/mini_test.hpp"
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
    void execute(Chunk*ch){
        printf("Chunk %p\n",ch);
    }
};
struct SpeedSystem : ISystem {
    JobChunkWrapper<SpeedJob> wrapper;
    SpeedSystem(DOE*e):wrapper{&e->ecs}{
        wrapper.query.withAllRW(getTypeID<Position>());
        wrapper.query.withAll(getTypeID<Speed>());
        wrapper.query.sort();
    }
    void OnUpdate(DOE*e){
        wrapper.schedule(e->dpm,e->tp);
    }
};

struct Test {
    static void Test1();
};
CLASS_TEST(Test,Test1){
    align_ptr<DOE> e = make_align<DOE>();
    SpeedSystem *system = allocator<SpeedSystem>().allocate(1);
    new (system) SpeedSystem(e.get());
    e->sys.emplace_back((ISystem*)system);
    Archetype *arch = e->ecs.getOrCreateArchetype(componentTypes<Entity,Speed,Position>());
    Entity entities[200];
    e->ecs.createEntities(arch,{entities,200});
    do {
        for(uint32_t i=0;i<e->sys.size();i++)
            e->sys[i]->OnUpdate(e.get());
        e->dpm.clear();
        e->tp.prepareJobs();
        e->tp.signalStart();
        while(!e->tp.isFinished());
    } while(false);
}

int main(){
    mtest::run_all();return 0;
}
