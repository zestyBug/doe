#include "ECS/Base/TypeID.hpp"

struct test_1 : ECS::ISharedComponentData, ECS::IManagedComponentData
{
    int test1;
    ~test_1(){
        //
    }
};


int main()
{
    ECS::TypeID type = ECS::TypeManager::registerType<test_1>("Test-1");
    type.Debug();
    return 0;
}
