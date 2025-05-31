#include "ECS/Archetype.hpp"
#include "cutil/prototype.hpp"

static int test_num=1;
#define DEF_TEST_SUBJECT() { printf("%i: ",test_num++);
#define END_TEST_SUBJECT(RES) printf((RES)?"Passed\n":"Failed\n");}

static_array<ECS::comp_info,32> ECS::rtti;

int main(int argc, char const *argv[])
{
    std::vector<ECS::TypeID> types;
    types.reserve(10);
    types.emplace_back(ECS::getTypeInfo<int>().value);
    types.emplace_back(ECS::getTypeInfo<prototype>().value);
    ECS::Archetype *arch = ECS::Archetype::createArchetype(types);

    DEF_TEST_SUBJECT()
    auto v1 = arch->createEntity(0);
    arch->removeEntity(v1);
    END_TEST_SUBJECT(true)

    DEF_TEST_SUBJECT()
    auto v1 = arch->createEntity(1);
    auto v2 = arch->createEntity(2);
    arch->removeEntity(v2);
    arch->removeEntity(v1);
    END_TEST_SUBJECT(true)

    DEF_TEST_SUBJECT()
    arch->createEntity(1);
    arch->createEntity(2);
    arch->createEntity(3);
    arch->removeEntity(0);
    arch->removeEntity(1);
    arch->removeEntity(0);
    END_TEST_SUBJECT(true)

    
    allocator<ECS::Archetype>().destroy(arch);
    allocator<ECS::Archetype>().deallocate(arch);
    return 0;
}
