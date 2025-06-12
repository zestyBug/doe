#include "ECS/Archetype.hpp"
#include "cutil/prototype.hpp"
#include "cutil/mini_test.hpp"

ECS::Archetype* arch;

TEST(Test1) {
    auto v1 = arch->createEntity(0);
    arch->removeEntity(v1);
}
TEST(Test2) {
    auto v1 = arch->createEntity(1);
    auto v2 = arch->createEntity(2);
    arch->removeEntity(v2);
    arch->removeEntity(v1);
}
TEST(Test3) {
    arch->createEntity(1);
    arch->createEntity(2);
    arch->createEntity(3);
    arch->removeEntity(0);
    arch->removeEntity(1);
    arch->removeEntity(0);
}

int main(int argc, char const *argv[])
{
    std::vector<ECS::TypeID> types;
    types.reserve(10);
    types.emplace_back(ECS::getTypeInfo<int>().value);
    types.emplace_back(ECS::getTypeInfo<prototype>().value);
    ECS::ArchetypeHolder h_arch = std::move(ECS::Archetype::createArchetype(types));
    arch = h_arch.get();

    mtest::run_all();
    
    allocator<ECS::Archetype>().destroy(arch);
    allocator<ECS::Archetype>().deallocate(arch);
    return 0;
}
