#include "src/Engine/defs.hpp"
#include "src/Engine/EntityCommandBuffer.hpp"
#include "src/Engine/ResourceManager.hpp"
#include "src/Engine/IteratorThread.hpp"

StaticArray<DOTS::comp_info,32> DOTS::rtti;

int main(int argc, char const *argv[])
{
    DOTS::ResourceManager rs;
    DOTS::IteratorThread<int,int> it([](const int &x){return (int)1;});
    return 0;
}
