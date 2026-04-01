#include "ECS/Base/TypeID.hpp"
#include "cutil/mini_test.hpp"

struct test_1 : ECS::ISharedComponentData, ECS::IManagedComponentData
{
    int test1;
    ~test_1(){
        //
    }
};
template<> ECS::TypeID ECS::__typeid__<test_1>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_1>("test_1");
    return v;
}

struct test_2 : ECS::IComponentData
{
};
template<> ECS::TypeID ECS::__typeid__<test_2>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_2>("test_2");
    return v;
}

TEST(Test1) {
    ECS::getTypeID<test_1>().Debug();
}
TEST(Test2) {
    ECS::getTypeID<test_2>().Debug();
}


int main()
{
    mtest::run_all();
    return 0;
}
