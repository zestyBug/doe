#include "cutil/bitset.hpp"
#include <stdio.h>
#include "cutil/mini_test.hpp"

TEST(Test1) {
    bitset_static<10> set1;
    bitset_static<10> set2;
    set2.set(15,1);
    set1.set(15,1);
#ifdef VERBOSE
    for(bool bit:set2)
        printf("%s",bit?"1":"0");
#endif

    EXPECT_EQ(set1,set2);
}

int main(int argc, char const *argv[])
{
    mtest::run_all();
    return 0;
}
