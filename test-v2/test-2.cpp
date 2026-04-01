#include "ECS/SharedComponentStore.hpp"
#include "cutil/mini_test.hpp"
#define COUNT 100
static int counter1 = 0;
static int counter2 = 0;
struct test_1 : ECS::ISharedComponentData, ECS::IManagedComponentData
{
    int test1;
    test_1(){
        test1 = counter1++;
    }
    ~test_1(){
        if(test1 != counter2)
            throw std::runtime_error("Unexpected value");
        counter2++;
    };
};
template<> ECS::TypeID ECS::__typeid__<test_1>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_1>("test_1");
    return v;
}

struct test_2 : ECS::ISharedComponentData, ECS::IManagedComponentData
{
    int test1;
    test_2(){
        test1 = 0;
    }
    ~test_2(){}
};
template<> ECS::TypeID ECS::__typeid__<test_2>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_2>("test_2");
    return v;
}


TEST(Test1) {
    {
        ECS::SharedComponentStore storage;
        auto v1 = storage.allocate(ECS::getTypeID<test_1>());
        auto v2 = storage.allocate(ECS::getTypeID<test_1>());
        EXPECT_EQ(counter1, 2);
    }
    EXPECT_EQ(counter2, 2);
}
// resize test
TEST(Test2) {
    {
        ECS::SharedComponentStore storage;
        for(uint32_t i=0;i<COUNT;i++)
            storage.allocate(ECS::getTypeID<test_1>());
    }
    EXPECT_EQ(counter1, counter2);
}
// deallocation
TEST(Test3) {
    {
        uint32_t buffer = counter1;
        ECS::SharedComponentStore storage;

        storage.allocate(ECS::getTypeID<test_2>());
        auto v1 = storage.allocate(ECS::getTypeID<test_2>());
        storage.allocate(ECS::getTypeID<test_2>());

        test_2 *ptr1 = (test_2*)storage.getPointer(v1);

        storage.refDecrease(v1,1);

        auto v2 = storage.allocate(ECS::getTypeID<test_2>());
        test_2 *ptr2 = (test_2*)storage.getPointer(v1);

        EXPECT_EQ(ptr1->test1, ptr2->test1);
    }
}

int main(){mtest::run_all();return 0;}
