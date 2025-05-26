#include "ECS/ListMap.hpp"
#include "../src/ListMap.cpp"

StaticArray<ECS::comp_info,32> ECS::rtti;

int main(int argc, char const *argv[])
{
    ECS::ListMap list;
    int test_num=1;
    #define DEF_TEST_SUBJECT() { printf("%i: ",test_num++);
    #define END_TEST_SUBJECT(RES) printf((RES)?"Passed\n":"Failed\n");}
    list.init(4);
    for (size_t i = 0; i < 120; i++)
    {
        list.add(i,i,i);
    }
    
    DEF_TEST_SUBJECT()    
        auto v = list.tryGetValue(29,29);
    END_TEST_SUBJECT(v == 29)

    DEF_TEST_SUBJECT()    
        auto v = list.tryGetValue(28,29);
    END_TEST_SUBJECT(v == list.invalideValue)
    
    DEF_TEST_SUBJECT()    
        auto v = list.indexOf(3,2);
    END_TEST_SUBJECT(v == -1)
    
    DEF_TEST_SUBJECT()    
        auto v = list.indexOf(3,3);
    END_TEST_SUBJECT(v == 3)
    
    DEF_TEST_SUBJECT()    
        auto v = list.tryGetValue(3,3);
    END_TEST_SUBJECT(v == 3)
    
    DEF_TEST_SUBJECT()    
        list.remove(3,3);
        auto v = list.tryGetValue(3,3);
    END_TEST_SUBJECT(v == list.invalideValue)
    
    DEF_TEST_SUBJECT()    
        auto v = list.add(3,3,3);
    END_TEST_SUBJECT(v == 3)

    for (size_t i = 100; i ; i--)
    {
        if(i & 0x7)
            continue;
        list.remove(i,i);
    }
    
    DEF_TEST_SUBJECT()    
        auto v = list.contains(110,110);
    END_TEST_SUBJECT(v == true)

    return 0;
}
