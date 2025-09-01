#include "ECS/ResourceGC.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "cutil/mini_test.hpp"


struct com1 {
    uint8_t a[8] = {0,0,0,0,0,0,0,0};
    intptr_t v=0;
};
struct com2 {
    intptr_t v=0;
};
struct com3 {
    intptr_t v=0;
};

static int dtor1_counter = 0;
static intptr_t dtor1_last_value;
void dtor1(intptr_t v){
    dtor1_counter++;
    dtor1_last_value = v;
    // printf("dtor(): %lld\n",v);
}

struct Test {
    static const_span<ECS::TypeID> components;
    static ECS::EntityComponentManager reg;
    static ECS::ResourceGC rgc;

    static void iterateChunks(const ECS::Archetype& archetype,
            uint32_t numBuffers,
            const uint16_t* sizeBuffer,
            const uint32_t* offsetBuffer) {
        const_span<align_ptr<ECS::Chunk>> archetypeChunks   = archetype.chunksData;
        const uint32_t lastChunkEntityCount = archetype.lastChunkEntityCount;
        const uint32_t chunkCapacity        = archetype.chunkCapacity;
        // may underflow!
        const uint32_t lastChunkIndex       = archetypeChunks.size() - 1;

    #ifdef DEBUG
        if(unlikely(  chunkCapacity == 0 ||
            (archetypeChunks.size() > 0 && lastChunkEntityCount == 0 )
        )) throw std::runtime_error("iterateChunks(): bad archetype!");
    #endif

        for(uint32_t chunckIndex = 0; chunckIndex < archetypeChunks.size(); chunckIndex++)
        {
            const uint8_t *chunkMemory = (const uint8_t *)archetypeChunks[chunckIndex]->memory;
            const uint32_t entityCount = chunckIndex == lastChunkIndex ? lastChunkEntityCount : chunkCapacity ;
        #ifdef DEBUG
            if(unlikely(chunkMemory == nullptr))
                throw std::runtime_error("iterateChunks(): found an uninitialized chunk in archetype!");
        #endif
            for (size_t index = 0; index < numBuffers; index++)
                rgc.searchMemory(chunkMemory + offsetBuffer[index], entityCount * sizeBuffer[index]);
        }
    }
    static void searchArchetype(const ECS::Archetype& archetype)
    {
        uint32_t number_of_found = 0;
        uint16_t size_buffer[components.size()];
        uint32_t offset_buffer[components.size()];
        {
            auto archetypeOffsets = archetype.getOffset();
            auto archetypeSizes = archetype.getSize();
            auto archetypeTypes = archetype.getType();
            uint32_t archetypeTypeIndex = 0;
            uint32_t componentIndex = 0;
            // fill offset_buffer
            while(archetypeTypeIndex < archetypeTypes.size() && componentIndex < components.size())
            {
                if(components[componentIndex].value == archetypeTypes[archetypeTypeIndex].value)
                {
                    size_buffer[number_of_found] = archetypeSizes[archetypeTypeIndex];
                    offset_buffer[number_of_found] = archetypeOffsets[archetypeTypeIndex];
                    ++number_of_found;
                    ++componentIndex;
                    ++archetypeTypeIndex;
                }
                else if(components[componentIndex].value < archetypeTypes[archetypeTypeIndex].value)
                    ++componentIndex;
                else
                    ++archetypeTypeIndex;
            }
        }
        if(number_of_found)
            iterateChunks(archetype,number_of_found,size_buffer,offset_buffer);
    }
    static void mark() {
        uint32_t archetypeIndex;
        for (archetypeIndex = 0; archetypeIndex < reg.archetypes.size(); archetypeIndex++)
            if(const ECS::Archetype *arch = reg.archetypes[archetypeIndex].get();arch)
                searchArchetype(*arch);
    }
    // Test0: when value wasnt here at all
    static void Test0();
    // Test1: when value is not accessible
    static void Test1();
    // Test2: when entity is not availabe
    static void Test2();
    // Test3: when value exists, but not in the way we expected
    static void Test3();
    // Test4: multi-component test 1
    static void Test4();
    // Test5: bad memory offset!
    static void Test5();
    // Test6: invalid memory iteration
    static void Test6();
    //Test():components{ECS::componentTypes<com1,com2>()},reg{},rgc{} {}
};

const_span<ECS::TypeID> Test::components = ECS::componentTypes<com1,com2>();
ECS::EntityComponentManager Test::reg;
ECS::ResourceGC Test::rgc;

CLASS_TEST(Test,Test0) {
    EXPECT_EQ(ECS::getTypeID<ECS::Entity>().realIndex(),0);
    EXPECT_EQ(dtor1_counter,0);

    for (size_t i = 0; i < 31; i++)
    {
        auto v0 = reg.createEntity(ECS::componentTypes<com1>());
        com1& val = reg.getComponent<com1>(v0);
        val.v = (int64_t)0xFF00000000000000;
    }

    rgc.add(dtor1,0xFF);
    rgc.add(dtor1,0xFE);
    rgc.add(dtor1,0xFD);
    rgc.add(dtor1,0xFC);
    rgc.add(dtor1,0xFB);
    rgc.add(dtor1,0xFA);
    rgc.add(dtor1,0xF9);
    rgc.add(dtor1,0xF8);
    rgc.add(dtor1,0xF7);
    rgc.add(dtor1,0xF6);
    rgc.add(dtor1,0xF5);

    rgc.add(dtor1,0xFF);
    rgc.add(dtor1,0xFE);
    rgc.add(dtor1,0xFD);
    rgc.add(dtor1,0xFC);
    rgc.add(dtor1,0xFB);
    rgc.add(dtor1,0xFA);
    rgc.add(dtor1,0xF9);
    rgc.add(dtor1,0xF8);
    rgc.add(dtor1,0xF7);
    rgc.add(dtor1,0xF6);
    rgc.add(dtor1,0xF5);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,11);
    EXPECT_EQ(rgc.occupiedNodes(),0u);
}

CLASS_TEST(Test,Test1) {
    // allocating some entitites, so chunks is not deleted entirly
    reg.createEntity(ECS::componentTypes<com1>());
    reg.createEntity(ECS::componentTypes<com1>());

    auto v0 = reg.createEntity(ECS::componentTypes<com1>());
    com1& val = reg.getComponent<com1>(v0);

    val.v = 0xFF;
    rgc.add(dtor1,0xFF);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),1u);

    val.v = 0x0;


    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,0xFFu);
    EXPECT_EQ(rgc.occupiedNodes(),0u);
}

CLASS_TEST(Test,Test2) {
    auto v0 = reg.createEntity(ECS::componentTypes<com1>());
    com1& val = reg.getComponent<com1>(v0);

    val.v = 0xFFEFFFFFFFFFFFFF;
    rgc.add(dtor1,0xFFEFFFFFFFFFFFFF);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),1u);

    reg.destroyEntity(v0);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,(intptr_t)0xFFEFFFFFFFFFFFFF);
    EXPECT_EQ(rgc.occupiedNodes(),0u);
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

CLASS_TEST(Test,Test3) {
    auto v0 = reg.createEntity(ECS::componentTypes<com1>());
    com1& val = reg.getComponent<com1>(v0);
    com1 v2;
    EXPECT_EQ(memcmp(&val,&v2,sizeof(com1)),0);

    val.v = 0xAA;
    rgc.add(dtor1,0xAA);
    rgc.add(dtor1,0x696969);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(rgc.occupiedNodes(),1u);

    val.v = 0x0;
    if(isLittleEndian())
        val.a[0]=0xAA;
    else
        val.a[7]=0xAA;

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),1u);




    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),1u);
}

CLASS_TEST(Test,Test4) {
    auto v0 = reg.createEntity(ECS::componentTypes<com1,com2>());
    auto v1 = reg.createEntity(ECS::componentTypes<com1,com2>());
    com1& val1 = reg.getComponent<com1>(v0);
    com2& val2 = reg.getComponent<com2>(v1);

    val1.v = 0x01234567;
    val2.v = 0x01234566;
    rgc.add(dtor1,0x01234567);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),2u);

    val1.v = 0x01234566;
    val2.v = 0x01234567;

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),2u);
}

CLASS_TEST(Test,Test5) {
    for (size_t i = 0; i < 2048; i++)
    {
        auto v1 = reg.createEntity(ECS::componentTypes<com1>());
        com1& val1 = reg.getComponent<com1>(v1);
        val1.v = (int64_t)0xFFFFEFFFFFFFFFFF;
    }

    auto v0 = reg.createEntity(ECS::componentTypes<com1>());
    com1& val0 = reg.getComponent<com1>(v0);
    val0.v = (int64_t)0xFFFFFFEFFFFFFFFF;
    rgc.add(dtor1,0xFFFFFFEFFFFFFFFF);

    for (size_t i = 0; i < 2048; i++)
    {
        auto v1 = reg.createEntity(ECS::componentTypes<com1>());
        com1& val1 = reg.getComponent<com1>(v1);
        val1.v = (int64_t)0xFFFFEFFFFFFFFFFF;
    }

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,0);
    EXPECT_EQ(rgc.occupiedNodes(),3u);

    val0.v = (int64_t)0xFFFFFFFEFFFFFFFF;

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,(intptr_t)0xFFFFFFEFFFFFFFFF);
    EXPECT_EQ(rgc.occupiedNodes(),2u);
}

CLASS_TEST(Test,Test6) {
    // enought entities to have multiple chunks
    for (size_t i = 0; i < 2048; i++)
    {
        auto v0 = reg.createEntity(ECS::componentTypes<com2,com3>());
        com2& val1 = reg.getComponent<com2>(v0);
        com3& val3= reg.getComponent<com3>(v0);
        val1.v = (int64_t)0xFFFFFFFEFFFFFFFF;
        val3.v = (int64_t)0xFFFFFFFFFFFFEFFF;
    }
    rgc.add(dtor1,0xFFFFFFFFFFFFEFFF);

    dtor1_counter = 0;
    mark();
    rgc.sweep();
    EXPECT_EQ(dtor1_counter,1);
    EXPECT_EQ(dtor1_last_value,(intptr_t)0xFFFFFFFFFFFFEFFF);
}

int main() {
    mtest::run_all();
    return 0;
}
