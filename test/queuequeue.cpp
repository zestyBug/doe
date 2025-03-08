#include "external/cutil/QueueQueue.hpp"
#include <thread>
#include <mutex>
#include <stdio.h>


QueueQueue<int,64> q;

std::mutex m;

size_t count;
void func1(){
    for (size_t i = 0; i < count; i++)
    {
        m.lock();
        q.push(i);
        m.unlock();
    }
    puts("s1");
}
void func2(){
    for (size_t i = 0; i < count*2; i++)
    {
        while (q.empty());
        int val = q.return_and_pop();
    }
    puts("s2");
    if(!q.empty()){
        puts("something went wrong");
    }

}

int main(int argc, char const *argv[])
{
    if(argc > 1)
        count = atoi(argv[1]);
    else
        count = 0xffff;
    std::thread t0(func1);
    std::thread t1(func1);
    std::thread t2(func2);
    t0.join();
    t1.join();
    t2.join();
    return 0;
}
