#include <iostream>
#include <thread>
#include <atomic>
#include "ECS/DependencyManager.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "ECS/ChunkJobFunction.hpp"
#include "cutil/mini_test.hpp"

ECS::EntityComponentManager *reg;
ECS::JobFilter f1;
std::atomic<uint32_t> counters[2];
ECS::TypeID types1[4];
ECS::Entity e1;


struct DummyJob1 :  ECS::ChunkJob
{
    void execute(span<void*> coms,uint32_t count){
        printf("thread %lld: dummmy job1 (CC: %u,EC: %u)\n", std::this_thread::get_id(),coms.size(),count);
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        counters[0].fetch_add(1,std::memory_order_relaxed);
    }
    const char* name(){
        return "DummyJob1";
    }
    ECS::JobFilter getFilter(){
        return f1;
    }
};

DummyJob1 j1;

class Test{
    static void cycle(ECS::ChunkJob *j){
        ECS::DependencyManager dm{};
        dm.ScheduleJob(j,{},reg->getVersion());
        reg->updateVersion();
        void* ctx = ECS::ChunkJobFunction::createContext(dm.registeredJobs,reg->archetypes,reg->getVersion());
        size_t buffer=UINT64_MAX;
        while(UINT64_MAX != (buffer=ECS::ChunkJobFunction::function(ctx,buffer)));
        ECS::ChunkJobFunction::destroyContext(ctx);
    }
public:
    /**
     * Beginer Test
     */
    static void test1();
    /**
     * getComponent Test
     */
    static void test2();
    static void test3();
};

CLASS_TEST(Test,test1) {
    cycle(&j1);
    cycle(&j1);
    counters[0] = 0;
    cycle(&j1);
    EXPECT_EQ(counters[0].load(), 0u);
}

CLASS_TEST(Test,test2) {

    counters[0] = 0;
    reg->getComponent<float>(e1) = 1.0;
    cycle(&j1);
    EXPECT_EQ(counters[0].load(), 1u);

    counters[0] = 0;
    cycle(&j1);
    EXPECT_EQ(counters[0].load(), 0u);
}

CLASS_TEST(Test,test3) {
    counters[0] = 0;
    reg->getComponent(e1,ECS::getTypeID<double>());
    cycle(&j1);
    EXPECT_EQ(counters[0].load(), 0u);
}

int main() {
    ECS::EntityComponentManager ecm;
    reg = &ecm;

    types1[0] = ECS::getTypeID<float>();
    types1[1] = ECS::getTypeID<int>();
    types1[2] = ECS::getTypeID<float>();
    //types1[3] = ECS::getTypeID<int>();
    f1.types = types1;
    f1.counts[0]=1;
    f1.counts[1]=1;
    f1.counts[2]=1;


    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,char>());
    ecm.createEntity(ECS::componentTypes<float,char>());
    ecm.createEntity(ECS::componentTypes<float,char>());
    ecm.createEntity(ECS::componentTypes<float,char>());
    ecm.createEntity(ECS::componentTypes<float,char,short>());
    ecm.createEntity(ECS::componentTypes<float,char,short>());
    ecm.createEntity(ECS::componentTypes<float,char,short>());
    ecm.createEntity(ECS::componentTypes<float,char,short>());
    ecm.createEntity(ECS::componentTypes<float,char,double>());
    ecm.createEntity(ECS::componentTypes<float,char,double>());
    ecm.createEntity(ECS::componentTypes<float,char,double>());
    ecm.createEntity(ECS::componentTypes<float,char,double>());
    e1 = ecm.createEntity(ECS::componentTypes<float,int,short>());
    ecm.createEntity(ECS::componentTypes<float,int,short>());
    ecm.createEntity(ECS::componentTypes<float,int,short>());
    ecm.createEntity(ECS::componentTypes<float,int,short>());
    ecm.createEntity(ECS::componentTypes<float,int,char>());
    ecm.createEntity(ECS::componentTypes<float,int,char>());
    ecm.createEntity(ECS::componentTypes<float,int,char>());
    ecm.createEntity(ECS::componentTypes<float,int,char>());
    ecm.createEntity(ECS::componentTypes<float,int,double>());
    ecm.createEntity(ECS::componentTypes<float,int,double>());
    ecm.createEntity(ECS::componentTypes<float,int,double>());
    ecm.createEntity(ECS::componentTypes<float,int,double>());

    mtest::run_all();
    return 0;
}
