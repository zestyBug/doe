#include <iostream>
#include "ECS/SystemManager.hpp"
#include "ECS/ThreadPool.hpp"

struct DummyJob :  ECS::ChunkJob
{
    void execute(span<void*>,uint32_t){
        printf("inside dummmy job\n");
    }
    DummyJob(/* args */){}
    ~DummyJob(){}
};


int main() {
    ECS::DependencyManager depManager;

    DummyJob job;

    {
        // Job A: writes to "Position"
        std::vector<ECS::TypeID> read = {1};
        std::vector<ECS::TypeID> write = {0};
        depManager.ScheduleJob(&job,(DummyJob::JobFunc)&DummyJob::execute,"J1", read, write);
    }
    {
        // Job A: writes to "Position"
        std::vector<ECS::TypeID> read = {0};
        std::vector<ECS::TypeID> write = {1};
        depManager.ScheduleJob(&job,(DummyJob::JobFunc)&DummyJob::execute,"J2", read, write);
    }
    {
        // Job A: writes to "Position"
        std::vector<ECS::TypeID> read = {};
        std::vector<ECS::TypeID> write = {0};
        depManager.ScheduleJob(&job,(DummyJob::JobFunc)&DummyJob::execute,"J3", read, write);
    }

    depManager.dummyExecute();
    return 0;
}
