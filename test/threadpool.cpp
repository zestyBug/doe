#include <iostream>
#include <atomic>
#include <chrono>
#include "ECS/ThreadPool.hpp"


static int test_num=1;
#define TEST_SUBJECT_BEGIN() { bool TEST_RESULT=false;
#define TEST_SUBJECT_END() printf("%i: %s %s:%d\n",test_num++,(!!(TEST_RESULT))?"✔":"✘",__FILE__,__LINE__);}

struct MyContext {
    std::atomic<int> counter = 0;
    int max = 100;
};

int jobFunction(void* context) {
    MyContext* ctx = static_cast<MyContext*>(context);
    int val = ctx->counter.fetch_add(1);
    if (val >= ctx->max) return 1; // done
    //printf("Working: %d\n",val);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 0;
}

int main() {
    ECS::ThreadPool pool(4);
    MyContext ctx;
    pool.submit(jobFunction, &ctx);

    pool.waitInloop();
    TEST_SUBJECT_BEGIN()
    TEST_RESULT = ctx.counter >= ctx.max;
    TEST_SUBJECT_END()
    return 0;
}
