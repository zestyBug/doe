#include "external/cutil/bitset.hpp"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    bitset set1{16};
    bitset set2{32};
    set2.set(15,1);
    set1.set(15,1);
    for(bool bit:set2){
        printf("%s",bit?"1":"0");
    }
    printf("\ntruesize of bitset 2: %u",set2.true_size());
    printf("\n1 ?== 2 : %i",set1==set2);
    return 0;
}
