#include <iostream>
#include "ECS/SystemManager.hpp"
#include "ThreadPool.hpp"

ECS::TypeID id = ECS::getTypeID<ECS::Entity>();

struct DummyJob :  ECS::ChunkJob
{
    void execute(span<void*>,uint32_t){
        printf("inside dummmy job\n");
    }
    const char* name(){
        return "DummyJob";
    }
    ECS::JobFilter getFilter(){
        ECS::JobFilter jf;
        jf.types = &id;
        jf.counts[0]=1;
        jf.counts[1]=0;
        jf.counts[2]=0;
        return jf;
    }
    DummyJob(/* args */){}
    ~DummyJob(){}
};


int main() {
    ECS::DependencyManager depManager;

    DummyJob job;
    depManager.ScheduleJob(&job);
    depManager.dummyExecute();
    return 0;
}
