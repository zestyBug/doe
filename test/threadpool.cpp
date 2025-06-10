#include <iostream>
#include <atomic>
#include <chrono>
#include "ECS/ThreadPool.hpp"
#include "cutil/mini_test.hpp"

struct MyContext {
    std::atomic<int> counter = 0;
    int max = 100;
};

size_t jobFunction(void* context, size_t) {
    MyContext* ctx = static_cast<MyContext*>(context);
    int val = ctx->counter.fetch_add(1);
    if (val >= ctx->max) return ECS::ThreadPool::STOP_SIGNAL; // done
    //printf("Working: %d\n",val);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 0;
}

TEST(Test) {
    ECS::ThreadPool pool(4);
    MyContext ctx;
    pool.submit(jobFunction, &ctx);

    pool.waitInloop();
    EXPECT_GE(ctx.counter,ctx.max);
}

int main() {
    mtest::run_all();
    return 0;
}
