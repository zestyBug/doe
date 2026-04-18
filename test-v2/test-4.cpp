#include "ECS/ThreadPool.hpp"
#include "cutil/mini_test.hpp"
#include <memory>
#include <iostream>
#include <new>


struct Test {
    static void Test1();
    static void Test2(void*,uint32_t begin,uint32_t end,ECS::JobHandle handle){
        auto v = std::this_thread::get_id();
        auto i = *(unsigned long int*)&v;
        printf("Job(%u ~ %u,%i,%lu);\n",begin,end,handle.index(),(i>>12)&0xF);
    }
};
CLASS_TEST(Test,Test1){
    using namespace ECS;
    ThreadPool tp(8);
    ThreadPool::JobData data = ThreadPool::JobData{
        .function = &Test::Test2,
        .context = nullptr,
        .batchCount = 1,
        .dependsOn = JobHandle(),
    };
    ECS::JobHandle js[10];

    js[0] = tp.submit(data);

    data.dependsOn = js[0];
    data.batchCount = 7;
    js[1] = tp.submit(data);

    data.dependsOn = js[1];
    data.batchCount = 7;
    js[2] = tp.submit(data);

    data.dependsOn = js[2];
    data.batchCount = 7;
    js[3] = tp.submit(data);

    data.dependsOn = js[0];
    data.batchCount = 7;
    js[4] = tp.submit(data);

    data.dependsOn = js[4];
    data.batchCount = 7;
    js[5] = tp.submit(data);

    tp.prepareJobs();
    tp.signalStart();
    while(!tp.isFinished())
        std::this_thread::yield();
}

int main(){
    mtest::run_all();return 0;
}
