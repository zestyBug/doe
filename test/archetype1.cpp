#include "ECS/Archetype.hpp"
#include "cutil/prototype.hpp"

static int test_num=1;
#define DEF_TEST_SUBJECT() { printf("%i: ",test_num++);
#define END_TEST_SUBJECT(RES) printf((RES)?"Passed\n":"Failed\n");}

StaticArray<DOTS::comp_info,32> DOTS::rtti;

int main(int argc, char const *argv[])
{
    std::vector<DOTS::TypeIndex> types;
    types.reserve(10);
    types.emplace_back(DOTS::getTypeInfo<int>().value);
    types.emplace_back(DOTS::getTypeInfo<prototype>().value);
    DOTS::Archetype *arch = DOTS::Archetype::createArchetype(types);

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

    
    allocator<DOTS::Archetype>().destroy(arch);
    allocator<DOTS::Archetype>().deallocate(arch);
    return 0;
}
