#include "src/Engine/ECS/defs.hpp"
#include "src/Engine/ECS/EntityCommandBuffer.hpp"
#include "src/Engine/ResourceManager/ResourceManager.hpp"
#include "src/Engine/IteratorThread.hpp"
#include <chrono>

StaticArray<DOTS::comp_info,32> DOTS::rtti;

int main()
{
    using namespace std::chrono_literals;
    DOTS::ResourceManager rs;
    DOTS::IteratorThread<int,int> it([](const int &x){printf("resource loading (%d)\n",x);return (int)1;});
    it.submit(5);
    it.submit(5);
    std::this_thread::sleep_for(8ns);
    while( it.processCallback([](const int &x){printf("resource callback (%d)\n",x);}) );
    return 0;
}
