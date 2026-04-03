#include "ECS/SharedComponentStore.hpp"
#include "cutil/mini_test.hpp"
#define COUNT 100
static int counter1 = 0;
static int counter2 = 0;
struct test_1 : ECS::ISharedComponentData, ECS::IManagedComponentData
{
    int var1 = 1;
    int var2 = 2;
    test_1(){
        counter1++;
    }
    ~test_1(){
        counter2++;
    };
};
template<> ECS::TypeID ECS::__typeid__<test_1>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_1>("test_1");
    return v;
}


TEST(Test1) {
    {
        ECS::SharedComponentStore storage;
        test_1 v1;
        test_1 v2;
        test_1 v3;
        test_1 v4;
        v3.var1 = 69;
        v4.var1 = 69;
        auto i1 = storage.insert(ECS::getTypeID<test_1>(),&v1);
        auto i2 = storage.insert(ECS::getTypeID<test_1>(),&v1);
        EXPECT_EQ((uint32_t)i1, (uint32_t)i2);
        auto i3 = storage.insert(ECS::getTypeID<test_1>(),&v2);
        EXPECT_EQ((uint32_t)i1, (uint32_t)i3);
        auto i4 = storage.insert(ECS::getTypeID<test_1>(),&v3);
        EXPECT_NE((uint32_t)i1, (uint32_t)i4);
        auto i5 = storage.insert(ECS::getTypeID<test_1>(),&v4);
        EXPECT_EQ((uint32_t)i4, (uint32_t)i5);
        storage.checkInternalConsistency();
    }
    EXPECT_EQ(counter1, 4);
    EXPECT_EQ(counter2, 6);
}

int main(){mtest::run_all();return 0;}
