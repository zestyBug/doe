//#include "ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "cutil/range.hpp"
#include "cutil/small_vector.hpp"
#include "cutil/prototype.hpp"
#include "cutil/mini_test.hpp"

ECS::EntityComponentManager *reg;
int prototype::counter = 0;

//ECS::ThreadPool *tp;

struct Hierarchy {
    ECS::Entity parent{};
    small_vector<ECS::Entity,8,allocator<ECS::Entity>> child{};
};



// ArchetypeVersionManager memory allocation test
TEST(Test1) {
    ECS::ArchetypeVersionManager chunks;
    chunks.initialize(4);
    for (size_t i = 0; i < 100; i++)
        chunks.add(0);
    chunks.popBack();
}
// remove all + add behaviour (single)
TEST(Test10) {
    auto v0 = reg->createEntity(ECS::componentTypes<Hierarchy>());
    reg->removeComponent(v0,ECS::getTypeInfo<Hierarchy>().value);
    EXPECT_EQ(reg->hasArchetype(v0),false);
    EXPECT_EQ(reg->hasComponent(v0,ECS::getTypeID<Hierarchy>()),false);
    reg->addComponent(v0,ECS::getTypeInfo<prototype>().value);
    EXPECT_NE(reg->hasComponent(v0,ECS::getTypeID<prototype>()),false);
    reg->destroyEntity(v0);
}
// remove all + add behaviour (multi)
TEST(Test11) {
    auto v0 = reg->createEntity(ECS::componentTypes<Hierarchy,int>());
    reg->removeComponents(v0,ECS::componentTypes<Hierarchy,int>());
    EXPECT_EQ(reg->hasArchetype(v0),false);
    EXPECT_EQ(reg->hasComponent(v0,ECS::getTypeID<Hierarchy>()),false);
    reg->addComponent(v0,ECS::getTypeInfo<prototype>().value);
    EXPECT_NE(reg->hasComponent(v0,ECS::getTypeID<prototype>()),false);
    reg->destroyEntity(v0);
}
// hasArchetype + valid functions test
TEST(Test12) {
    auto v0 = reg->createEntity(ECS::componentTypes<Hierarchy,int>());
    reg->removeComponents(v0,ECS::componentTypes<Hierarchy,int>());

    EXPECT_EQ(reg->hasArchetype(v0),false);
    EXPECT_NE(reg->valid(v0),false);
    

    reg->addComponent(v0,ECS::getTypeInfo<prototype>().value);
    
    EXPECT_NE(reg->hasArchetype(v0),false);
    EXPECT_NE(reg->valid(v0),false);

    reg->destroyEntity(v0);
}
// simple getComponent + hasArchetype test
TEST(Test30) {
    auto v1 = reg->createEntity(ECS::componentTypes<Hierarchy,int>());
    EXPECT_NE(reg->getComponent(v1,ECS::getTypeID<int>()),nullptr);
    EXPECT_NE(reg->getComponent(v1,ECS::getTypeID<Hierarchy>()),nullptr);
    EXPECT_EQ(reg->getComponent(v1,ECS::getTypeID<prototype>()),nullptr);
    reg->removeComponents(v1,ECS::componentTypes<Hierarchy,int>());
    EXPECT_EQ(reg->hasArchetype(v1),false);
    reg->destroyEntity(v1);
}
// add existing component behaviour
TEST(Test20) {
    const ECS::EntityComponentManager* creg = reg;
    auto v4 = reg->createEntity(ECS::componentTypes<Hierarchy,float>());
    const ECS::Archetype *arch = creg->getArchetype(v4);
    EXPECT_NE(arch,nullptr);
    reg->addComponent(v4,ECS::getTypeInfo<Hierarchy>().value);
    EXPECT_EQ(arch,creg->getArchetype(v4));
    reg->addComponents(v4,ECS::componentTypes<Hierarchy,float>());
    EXPECT_EQ(arch,creg->getArchetype(v4));
    reg->destroyEntity(v4);
}
// add prototype test
TEST(Test40) {
    auto v2 = reg->createEntity(ECS::componentTypes<Hierarchy,char>());
    reg->addComponents(v2,ECS::componentTypes<Hierarchy,prototype>());
    reg->destroyEntity(v2);
}
// iteration
TEST(Test50) {
    reg->iterate<int>([](const_span<void*>,uint32_t count){
    #ifdef VERBOSE
        printf("int count: %u\n",count);
    #endif
    });
}


int main()
{
    
    reg = new ECS::EntityComponentManager();
    
    
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy>());
    }
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,int>());
    }
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,char>());
    }
    
    mtest::run_all();
    delete reg;
#ifdef DEBUG
    printf("counter: %li\n",allocator_counter);
#endif
    return 0;
}