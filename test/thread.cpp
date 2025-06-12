#include <iostream>
#include "ECS/DependencyManager.hpp"
#include "ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "ECS/ChunkJobFunction.hpp"
#include <unistd.h>

static int test_num=1;
#define TEST_SUBJECT_BEGIN() { bool TEST_RESULT=false;
#define TEST_SUBJECT_END() printf("%i: %s %s:%d\n",test_num++,(!!(TEST_RESULT))?"✔":"✘",__FILE__,__LINE__);}

ECS::JobFilter f1, f2, f3;
std::atomic<uint32_t> counters[3];

struct DummyJob1 :  ECS::ChunkJob
{
    void execute(span<void*> coms,uint32_t count){
        printf("thread %lld: dummmy job1 (CC: %u,EC: %u)\n", std::this_thread::get_id(),coms.size(),count);
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        counters[0].fetch_add(1,std::memory_order_relaxed);
    }
    const char* name(){
        return "DummyJob";
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
        return "DummyJob";
    }
    ECS::JobFilter getFilter(){
        return f2;
    }
};

struct DummyJob3 :  ECS::ChunkJob
{
    void execute(span<void*> coms,uint32_t count){
        printf("thread %lld: dummmy job3 (CC: %u,EC: %u)\n", std::this_thread::get_id(),coms.size(),count);
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        counters[2].fetch_add(1,std::memory_order_relaxed);
    }
    const char* name(){
        return "DummyJob";
    }
    ECS::JobFilter getFilter(){
        return f3;
    }
};

void Test()
{
    ECS::TypeID types1[2],types2[2],types3[2];

    types1[0] = ECS::getTypeID<float>();
    types1[1] = ECS::getTypeID<int>();

    types2[0] = ECS::getTypeID<float>();
    types2[1] = ECS::getTypeID<char>();

    types3[0] = ECS::getTypeID<short>();
    types3[1] = ECS::getTypeID<int>();

    f1.types = types1;
    f1.counts[0]=1;
    f1.counts[1]=1;
    f1.counts[2]=0;

    f2.types = types2;
    f2.counts[0]=1;
    f2.counts[1]=1;
    f2.counts[2]=0;

    f3.types = types3;
    f3.counts[0]=1;
    f3.counts[1]=1;
    f3.counts[2]=0;

    ECS::EntityComponentManager ecm;
    ecm.createEntity(ECS::componentTypes<float,unsigned char,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned char,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned char,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned char,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned short,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned short,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned short,int>());
    ecm.createEntity(ECS::componentTypes<float,unsigned short,int>());
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
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
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



    ECS::DependencyManager dm{};
    DummyJob1 j1;
    DummyJob2 j2;
    DummyJob3 j3;

    dm.ScheduleJob(&j1);
    dm.ScheduleJob(&j2);
    dm.ScheduleJob(&j3);

    void* ctx = ECS::ChunkJobFunction::createContext(dm.registeredJobs,ecm.archetypes,0);
    {
        ECS::ThreadPool tp{8};
        tp.submit(ECS::ChunkJobFunction::function,ctx);
        tp.waitInloop();
    }
    TEST_SUBJECT_BEGIN()
    TEST_RESULT = counters[0].load() == 6;
    TEST_SUBJECT_END()

    TEST_SUBJECT_BEGIN()
    TEST_RESULT = counters[1].load() == 4;
    TEST_SUBJECT_END()

    TEST_SUBJECT_BEGIN()
    TEST_RESULT = counters[2].load() == 2;
    TEST_SUBJECT_END()

    ECS::ChunkJobFunction::destroyContext(ctx);

}
int main(int argc, char const *argv[])
{
    Test();
    return 0;
}
