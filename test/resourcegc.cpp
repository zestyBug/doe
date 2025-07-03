#include "ECS/ResourceGC.hpp"
#include "cutil/mini_test.hpp"
ECS::EntityComponentManager *reg;
ECS::ResourceGC<uint32_t> *rgc;

struct com1 {
    uint8_t a[4] = {0,0,0,0};
    int32_t v=0;
};
struct com2 {
    int32_t v=0;
};
struct com3 {
    int32_t v=0;
};

static int dtor1_counter = 0;
static uint32_t dtor1_last_value;
void dtor1(uint32_t v){
    dtor1_counter++;
    dtor1_last_value = v;
}

// Test0: when value wasnt here at all
TEST(Test0) {
    for (size_t i = 0; i < 31; i++)
    {
        auto v0 = reg->createEntity(ECS::componentTypes<com1>());
        com1& val = reg->getComponent<com1>(v0);
        val.v = (int32_t)0xFF000000;
    }

    rgc->add(dtor1,0xFF);
    rgc->add(dtor1,0xFE);

    rgc->add(dtor1,0xFD);
    rgc->add(dtor1,0xFC);

    rgc->add(dtor1,0xFB);
    rgc->add(dtor1,0xFA);

    rgc->add(dtor1,0xFF);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,6);
    EXPECT_EQ(rgc->occupiedNodes(),0u);
    dtor1_counter = 0;
}

// Test1: when value is not accessible
TEST(Test1) {
    // allocating some entitites, so chunks is not deleted entirly
    reg->createEntity(ECS::componentTypes<com1>());
    reg->createEntity(ECS::componentTypes<com1>());

    auto v0 = reg->createEntity(ECS::componentTypes<com1>());
    com1& val = reg->getComponent<com1>(v0);

    val.v = 0xFF;
    rgc->add(dtor1,0xFF);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc->occupiedNodes(),1u);

    val.v = 0x0;


    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0xFF);
    EXPECT_EQ(rgc->occupiedNodes(),0u);

    dtor1_counter = 0;
}
// Test2: when entity is not availabe
TEST(Test2) {
    auto v0 = reg->createEntity(ECS::componentTypes<com1>());
    com1& val = reg->getComponent<com1>(v0);

    val.v = 0x7EFEFEFE;
    rgc->add(dtor1,0x7EFEFEFE);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc->occupiedNodes(),1u);

    reg->destroyEntity(v0);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0x7EFEFEFE);
    EXPECT_EQ(rgc->occupiedNodes(),0u);

    dtor1_counter = 0;
}

bool isLittleEndian() {
    int i = 1;
    // Cast the address of the integer to a char pointer.
    // A char pointer points to a single byte.
    char* c = reinterpret_cast<char*>(&i);

    // If the system is little-endian, the least significant byte (which is 1 in this case)
    // will be stored at the lowest memory address, pointed to by 'c'.
    // If the system is big-endian, the most significant byte (0) will be at the lowest address.
    return (*c == 1);
}

// Test3: when value exists, but not in the way we expected
TEST(Test3) {
    auto v0 = reg->createEntity(ECS::componentTypes<com1>());
    com1& val = reg->getComponent<com1>(v0);

    val.v = 0x12345678;
    rgc->add(dtor1,0x12345678);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc->occupiedNodes(),1u);

    val.v = 0x0;
    if(isLittleEndian())
        val.a[0]=0xAA;
    else
        val.a[3]=0xAA;

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0x12345678);
    EXPECT_EQ(rgc->occupiedNodes(),0);
    dtor1_counter = 0;
}

// Test4: multi-component test 1
TEST(Test4) {
    auto v0 = reg->createEntity(ECS::componentTypes<com1,com2>());
    auto v1 = reg->createEntity(ECS::componentTypes<com1,com2>());
    com1& val1 = reg->getComponent<com1>(v0);
    com2& val2 = reg->getComponent<com2>(v1);

    val1.v = 0x01234567;
    val2.v = 0x01234566;
    rgc->add(dtor1,0x01234567);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc->occupiedNodes(),1u);

    val1.v = 0x01234566;
    val2.v = 0x01234567;

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc->occupiedNodes(),1u);

    reg->destroyEntity(v1);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0x01234567);
    EXPECT_EQ(rgc->occupiedNodes(),0u);
    dtor1_counter = 0;
}

// Test5: bad memory offset!
TEST(Test5) {
    for (size_t i = 0; i < 2048; i++)
    {
        auto v1 = reg->createEntity(ECS::componentTypes<com1>());
        com1& val1 = reg->getComponent<com1>(v1);
        val1.v = (int32_t)0xFFFFFFFE;
    }

    auto v0 = reg->createEntity(ECS::componentTypes<com1>());
    com1& val0 = reg->getComponent<com1>(v0);
    val0.v = (int32_t)0xFFFFFFEF;
    rgc->add(dtor1,0xFFFFFFEF);

    for (size_t i = 0; i < 2048; i++)
    {
        auto v1 = reg->createEntity(ECS::componentTypes<com1>());
        com1& val1 = reg->getComponent<com1>(v1);
        val1.v = (int32_t)0xFFFFFFFE;
    }

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,0);

    val0.v = (int32_t)0xFFFFFFFE;

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0xFFFFFFEF);
    dtor1_counter = 0;
}

// Test6: invalid memory iteration
TEST(Test6) {
    // enought entities to have multiple chunks
    for (size_t i = 0; i < 2048; i++)
    {
        auto v0 = reg->createEntity(ECS::componentTypes<com2,com3>());
        com2& val1 = reg->getComponent<com2>(v0);
        com3& val3= reg->getComponent<com3>(v0);
        val1.v = (int32_t)0xFFFFFFFE;
        val3.v = (int32_t)0xFFFFFFEF;
    }
    rgc->add(dtor1,0xFFFFFFEF);

    rgc->mark(reg);
    rgc->sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0xFFFFFFEF);
    dtor1_counter = 0;
}

int main() {
    ECS::EntityComponentManager _ecm;
    ECS::ResourceGC<uint32_t> _rgc{ECS::componentTypes<com1,com2>()};
    reg = &_ecm;
    rgc = &_rgc;
    mtest::run_all();
    return 0;
}
