#include "cutil/map.hpp"
#include "ECS/Archetype.hpp"
#include "cutil/unique_ptr.hpp"

static int test_num=1;
#define DEF_TEST_SUBJECT() { printf("%i:",test_num++);
#define END_TEST_SUBJECT(RES) printf("%s %s:%d\n",(RES)?"✔":"✘",__FILE__,__LINE__);}

int main(int argc, char const *argv[])
{
    map<const_span<ECS::TypeID>,ECS::Archetype> list;
    list.init(4);
    std::vector<ECS::ArchetypeHolder> archs;

    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<int>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<float>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<char>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<int,float>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<int,char>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<int,short>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<char,short>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<float,short>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<float,char,short>()));
    archs.emplace_back(ECS::Archetype::createArchetype(ECS::componentTypes<float,int,short>()));

    list.add(archs[0].get());
    list.add(archs[1].get());
    list.add(archs[2].get());
    list.add(archs[3].get());
    list.add(archs[4].get());
    list.add(archs[5].get());
    list.add(archs[6].get());
    
    DEF_TEST_SUBJECT()    
        ECS::Archetype *v = list.tryGet(ECS::componentTypes<int>());
    END_TEST_SUBJECT(v == archs[0].get())

    DEF_TEST_SUBJECT()    
        ECS::Archetype *v = list.tryGet(ECS::componentTypes<int,char>());
    END_TEST_SUBJECT(v == archs[4].get())

    list.remove(archs[0].get());
    list.remove(archs[4].get());
    list.remove(archs[1].get());
    list.remove(archs[2].get());
    list.remove(archs[3].get());
    
    DEF_TEST_SUBJECT()    
        bool v = list.contains(archs[5].get());
    END_TEST_SUBJECT(v == true)

    return 0;
}
