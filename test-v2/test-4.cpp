#include "ECS/ThreadPool.hpp"
#include "cutil/mini_test.hpp"
#include <memory>
#include <iostream>
#include <new>
#include <unistd.h>
#include <mutex>

std::mutex x;

struct Test {
    static void Test1();
    static void Test2(void*,uint32_t,uint32_t,ECS::JobHandle handle){
        auto v = std::this_thread::get_id();
        auto i = *(unsigned long int*)&v;
        x.lock();
        printf("Job(%i,%lu);\n",handle.index(),(i>>12)&0xF);
        x.unlock();
        usleep(500000);
    }
};
CLASS_TEST(Test,Test1){
    using namespace ECS;
    JobParameter data = JobParameter{
        .function = &Test::Test2,
        .context = nullptr,
        .batchCount = 1,
        .dependsOn = JobHandle(),
    };
    ECS::JobHandle js[10];

    js[0] = JobsUtility.schedule(data);

    data.dependsOn = js[0];
    data.batchCount = 20;
    js[1] = JobsUtility.schedule(data);

    data.dependsOn = js[1];
    data.batchCount = 20;
    js[2] = JobsUtility.schedule(data);

    data.dependsOn = js[2];
    data.batchCount = 20;
    js[3] = JobsUtility.schedule(data);

    data.dependsOn = js[0];
    data.batchCount = 20;
    js[4] = JobsUtility.schedule(data);

    data.dependsOn = js[4];
    data.batchCount = 20;
    js[5] = JobsUtility.schedule(data);

    data.dependsOn = JobsUtility.combineDependencies({js,6});;
    data.batchCount = 20;
    js[6] = JobsUtility.schedule(data);

    JobsUtility.prepareJobs();
    JobsUtility.signalStart();
    while(!JobsUtility.isFinished())
        std::this_thread::yield();
}

int main(){
    ECS::JobsUtility.INIT(8);
    mtest::run_all();return 0;
}
