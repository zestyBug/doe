#include <iostream>
#include <thread>
#include <atomic>
#include "ECS/DependencyManager.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "ECS/ChunkJobFunction.hpp"
#include "cutil/mini_test.hpp"

ECS::EntityComponentManager *reg;
ECS::JobFilter f1, f2;
std::atomic<uint32_t> counters[2];
ECS::TypeID types1[4],types2[4];

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

struct DummyJob2 :  ECS::ChunkJob
{
    void execute(span<void*> coms,uint32_t count){
        printf("thread %lld: dummmy job2 (CC: %u,EC: %u)\n", std::this_thread::get_id(),coms.size(),count);
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        counters[1].fetch_add(1,std::memory_order_relaxed);
    }
    const char* name(){
        return "DummyJob2";
    }
    ECS::JobFilter getFilter(){
        return f2;
    }
};




TEST(Test) {
    void* ctx;
    DummyJob1 j1;

    {
        ECS::DependencyManager dm{};
        dm.ScheduleJob(&j1,{},0);
        ctx = ECS::ChunkJobFunction::createContext(dm.registeredJobs,reg->archetypes,1);
        size_t buffer=UINT64_MAX;
        while(UINT64_MAX != (buffer=ECS::ChunkJobFunction::function(ctx,buffer)));
        ECS::ChunkJobFunction::destroyContext(ctx);
    }

    {
        ECS::DependencyManager dm{};
        dm.ScheduleJob(&j1,{},1);
        ctx = ECS::ChunkJobFunction::createContext(dm.registeredJobs,reg->archetypes,2);
        size_t buffer=UINT64_MAX;
        while(UINT64_MAX != (buffer=ECS::ChunkJobFunction::function(ctx,buffer)));
        ECS::ChunkJobFunction::destroyContext(ctx);
    }
    counters[0] = 0;
    {
        ECS::DependencyManager dm{};
        dm.ScheduleJob(&j1,{},2);
        ctx = ECS::ChunkJobFunction::createContext(dm.registeredJobs,reg->archetypes,3);
        size_t buffer=UINT64_MAX;
        while(UINT64_MAX != (buffer=ECS::ChunkJobFunction::function(ctx,buffer)));
        ECS::ChunkJobFunction::destroyContext(ctx);
    }

    EXPECT_EQ(counters[0].load(), 0u);
}

int main() {
    ECS::EntityComponentManager ecm;
    reg = &ecm;

    types1[0] = ECS::getTypeID<float>();
    types1[1] = ECS::getTypeID<int>();
    types1[2] = ECS::getTypeID<float>();
    types1[3] = ECS::getTypeID<int>();

    types2[0] = ECS::getTypeID<int>();

    f1.types = types1;
    f1.counts[0]=1;
    f1.counts[1]=1;
    f1.counts[2]=1;

    f2.types = types2;
    f2.counts[0]=0;
    f2.counts[1]=1;
    f2.counts[2]=0;

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
    ecm.createEntity(ECS::componentTypes<float,int,short>());
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
