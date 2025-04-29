#include "external/cutil/bitset.hpp"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    bitset_static<10> set1;
    bitset_static<10> set2;
    set2.set(15,1);
    set1.set(15,1);
    for(bool bit:set2){
        printf("%s",bit?"1":"0");
    }

    printf("\n1 ?== 2 : %i",set1==set2);
    return 0;
}
