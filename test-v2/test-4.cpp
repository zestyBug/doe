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
    static void Test2(void*,uint32_t begin,uint32_t end,ECS::JobHandle handle){
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
    ThreadPool tp(8);
    ThreadPool::JobParameter data = ThreadPool::JobParameter{
        .function = &Test::Test2,
        .context = nullptr,
        .batchCount = 1,
        .dependsOn = JobHandle(),
    };
    ECS::JobHandle js[10];

    js[0] = tp.schedule(data);

    data.dependsOn = js[0];
    data.batchCount = 20;
    js[1] = tp.schedule(data);

    data.dependsOn = js[1];
    data.batchCount = 20;
    js[2] = tp.schedule(data);

    data.dependsOn = js[2];
    data.batchCount = 20;
    js[3] = tp.schedule(data);

    data.dependsOn = js[0];
    data.batchCount = 20;
    js[4] = tp.schedule(data);

    data.dependsOn = js[4];
    data.batchCount = 20;
    js[5] = tp.schedule(data);

    data.dependsOn = tp.combineDependencies({js,6});;
    data.batchCount = 20;
    js[6] = tp.schedule(data);

    tp.prepareJobs();
    tp.signalStart();
    while(!tp.isFinished())
        std::this_thread::yield();
}

int main(){
    mtest::run_all();return 0;
}
