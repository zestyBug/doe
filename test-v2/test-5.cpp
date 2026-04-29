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
        Archetype *arch = ch->archetype;
        uint32_t count = ch->entityCount;
        const uint32_t index1 = arch->getIndexInTypeArray(getTypeID<Speed>());
        const uint32_t index2 = arch->getIndexInTypeArray(getTypeID<Position>());
        Speed    *speedArray    = (Speed*)   arch->getComponentDataRO(ch,0,index1);
        Position *positionArray = (Position*)arch->getComponentDataRO(ch,0,index2);
        while(count--){
            positionArray->x += speedArray->x;
            ++speedArray;
            ++positionArray;
        }
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
    uint32_t counter=100000;
    do {
        for(uint32_t i=0;i<e->sys.size();i++)
            e->sys[i]->OnUpdate(e.get());
        e->dpm.clear();
        e->tp.prepareJobs();
        e->tp.signalStart();
        while(!e->tp.isFinished())
            std::this_thread::yield();
        e->tp.reset();
    } while(--counter);
}

int main(){
    mtest::run_all();return 0;
}
