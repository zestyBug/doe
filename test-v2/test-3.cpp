#include "ECS/EntityComponentStore.hpp"
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
        counter1--;
    };
};
template<> ECS::TypeID ECS::__typeid__<test_1>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_1>("test_1");
    return v;
}
struct test_2 : ECS::IComponentData, ECS::IManagedComponentData
{
    int var1 = 1;
    int var2 = 2;
    test_2(){
        counter2++;
    }
    ~test_2(){
        counter2--;
    }
};
template<> ECS::TypeID ECS::__typeid__<test_2>(){
    static ECS::TypeID v = ECS::TypeManager::registerType<test_2>("test_2");
    return v;
}


struct Test {
    static void Test1();
};

void Test::Test1() {
    using namespace ECS;
    align_ptr<EntityComponentStore> store = EntityComponentStore::create();
    Archetype *arch = store->getOrCreateArchetype(componentTypes<Entity,test_2,test_1>());
    Entity entities[10];
    SharedComponentIndex index = store->sharedComponents.getDefaultValue(getTypeID<test_1>());
    store->createEntities(arch,{entities,10},{&index,0});
    store->removeComponent(entities[1], getTypeID<test_2>());
    store->removeComponent(entities[1], getTypeID<test_1>());
    store->removeComponent(entities[2], getTypeID<test_1>());
}

int main(){
    Test::Test1();
    //mtest::run_all();return 0;
}
