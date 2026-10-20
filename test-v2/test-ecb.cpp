#include "ECS/EntityCommandBuffer.hpp"
#include "ECS/Engine.hpp"
#include "cutil/mini_test.hpp"
#define COUNT 100
static int counter1 = 0;
static int counter2 = 0;

struct comp_1 : ECS::ISharedComponentData
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


TEST(Test1) {
    ECS::EntityCommandBuffer ecb;
    ECS::DOE engine;
    ECS::EntityQueryBuilder eqb;
    ECS::Entity e = ecb.createEntity();
    ecb.addComponent<comp_1>(e);
    ecb.addComponentValue<comp_2>(e,comp_2(67,69));
    ecb.playback(engine.ecs);
    eqb.withAll(ECS::getTypeID<comp_2>());
    ECS::EntityQueryImpl query = engine.eqm.createEntityQuery(eqb, engine.ecs);
    engine.eqm.iterate(engine.ecs, query, [](span<const void *> args){
        comp_2 &com = *(comp_2*)args.at(0);
        counter1 = com.var1;
        counter2 = com.var2;
    });
    engine.ecs.checkInternalConsistency();
    EXPECT_EQ(counter1, 67);
    EXPECT_EQ(counter2, 69);
}

int main(){mtest::run_all();return 0;}
