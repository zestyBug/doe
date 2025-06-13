#include <iostream>
#include "ECS/SystemManager.hpp"
#include "ThreadPool.hpp"
#include "cutil/mini_test.hpp"

ECS::TypeID id = ECS::getTypeID<ECS::Entity>();
uint32_t counter = 0;
struct DummyJob :  ECS::ChunkJob
{
    void execute(span<void*>,uint32_t){
    #ifdef VERBOSE
        printf("inside dummmy job\n");
    #endif
        counter++;
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

TEST(Test1) {
    ECS::DependencyManager depManager;
    DummyJob job;
    depManager.ScheduleJob(&job);
    depManager.dummyExecute();
    EXPECT_EQ(counter,1u);
}


int main() {
    mtest::run_all();
    return 0;
}
