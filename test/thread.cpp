#include <iostream>
#include "ECS/SystemManager.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"


static int test_num=1;
#define DEF_TEST_SUBJECT() { printf("%i:",test_num++);
#define END_TEST_SUBJECT(RES) printf("%s %s:%d\n",(RES)?"✔":"✘",__FILE__,__LINE__);}

struct DummyJob :  ECS::ChunkJob
{
    void execute(span<void*> coms,uint32_t count){
        printf("thread %lld: dummmy job (CC: %u,EC: %u)\n",std::this_thread::get_id(),coms.size(),count);
    }
    DummyJob(/* args */){}
    ~DummyJob(){}
};

struct DummySyetem : public ECS::System {
    DummyJob j;
    void onUpdate(ECS::SystemState& ss){
        printf("from DummySyetem!\n");
        ss.jobs.ScheduleJob(&j,(ECS::ChunkJob::JobFunc)&DummyJob::execute,"DummyJob",ECS::componentTypes<int,short>());
    }
};

int main()
{
    ECS::EntityComponentManager ecm;
    ECS::ThreadPool tp{4};
    ECS::SystemManager sm;
    ECS::SystemState ss;

    auto s = sm.systems.create();
    sm.systems.emplace(s,new DummySyetem());

    sm.updateAll(ss);
    ss.jobStatueBuffer.resize(ss.jobs.registeredJobs.size());

    ecm.createEntity(ECS::componentTypes<float,double,int>());
    ecm.createEntity(ECS::componentTypes<float,double,int>());
    ecm.createEntity(ECS::componentTypes<float,double,int>());
    ecm.createEntity(ECS::componentTypes<float,double,int>());
    ecm.createEntity(ECS::componentTypes<float,double,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<float,int>());
    ecm.createEntity(ECS::componentTypes<int>());
    ecm.createEntity(ECS::componentTypes<int>());
    ecm.createEntity(ECS::componentTypes<int>());
    ecm.createEntity(ECS::componentTypes<int>());
    ecm.createEntity(ECS::componentTypes<int>());
    ecm.createEntity(ECS::componentTypes<int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<float,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,short,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<double,char,int>());
    ecm.createEntity(ECS::componentTypes<char,int>());
    ecm.createEntity(ECS::componentTypes<char,int>());
    ecm.createEntity(ECS::componentTypes<char,int>());
    ecm.createEntity(ECS::componentTypes<char,int>());
    ecm.createEntity(ECS::componentTypes<char,int>());

    tp.init(ss,ecm);
    tp.restart();

    while (true);
    

    return 0;
}
