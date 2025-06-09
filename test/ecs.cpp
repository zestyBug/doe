//#include "Engine/ECS/ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "cutil/range.hpp"
#include "cutil/small_vector.hpp"
#include "cutil/prototype.hpp"
ECS::EntityComponentManager *reg;
int prototype::counter = 0;

void Test();

static int test_num=1;
#define TEST_SUBJECT_BEGIN() { bool TEST_RESULT=false;
#define TEST_SUBJECT_END() printf("%i: %s %s:%d\n",test_num++,(!!(TEST_RESULT))?"✔":"✘",__FILE__,__LINE__);}

//ECS::ThreadPool *tp;

struct Hierarchy {
    ECS::Entity parent{};
    small_vector<ECS::Entity,8,allocator<ECS::Entity>> child{};
};

void Test(){
    reg = new ECS::EntityComponentManager();

    {
        ECS::ArchetypeVersionManager chunks;
        chunks.initialize(4);
        for (size_t i = 0; i < 100; i++)
        {
            chunks.add(0);
        }
        chunks.popBack();
    }

    auto v0 = reg->createEntity(ECS::componentTypes<Hierarchy>());
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy>());
    }
    auto v1 = reg->createEntity(ECS::componentTypes<Hierarchy,int>());
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,int>());
    }
    auto v2 = reg->createEntity(ECS::componentTypes<Hierarchy,char>());
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,char>());
    }
    auto v4 = reg->createEntity(ECS::componentTypes<Hierarchy,float>());
    
    TEST_SUBJECT_BEGIN()
    reg->removeComponent(v0,ECS::getTypeInfo<Hierarchy>().value);
    TEST_RESULT = !reg->hasArchetype(v0);
    TEST_RESULT &= !reg->hasComponent(v0,ECS::getTypeID<Hierarchy>());
    reg->addComponent(v0,ECS::getTypeInfo<prototype>().value);
    TEST_RESULT &= reg->hasComponent(v0,ECS::getTypeID<prototype>());
    reg->destroyEntity(v0);
    TEST_SUBJECT_END()

    TEST_SUBJECT_BEGIN()
    reg->removeComponents(v1,ECS::componentTypes<Hierarchy,int>());
    TEST_RESULT = !reg->hasArchetype(v1);
    reg->destroyEntity(v1);
    TEST_SUBJECT_END()

    TEST_SUBJECT_BEGIN()
    ECS::Archetype *arch;
    arch = reg->getArchetype(v4);
    TEST_RESULT = arch != nullptr;
    reg->addComponent(v4,ECS::getTypeInfo<Hierarchy>().value);
    TEST_RESULT &= arch == reg->getArchetype(v4);
    reg->addComponents(v4,ECS::componentTypes<Hierarchy,float>());
    TEST_RESULT &= arch == reg->getArchetype(v4);
    reg->destroyEntity(v4);
    TEST_SUBJECT_END()

    reg->addComponents(v2,ECS::componentTypes<Hierarchy,prototype>());
    reg->destroyEntity(v2);
    
    reg->iterate<int>([](span<void*>,uint32_t count){
        printf("int count: %u\n",count);
    });

    delete reg;
#ifdef DEBUG
    printf("counter: %li\n",allocator_counter);
#endif
}
int main()
{
    Test();
    return 0;
}