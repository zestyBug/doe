#include "Engine/ECS/ThreadPool.hpp"
#include "Engine/ECS/Register.hpp"
#include "Engine/ECS/System.hpp"
#include "cutil/Range.hpp"
#include "cutil/SmallVector.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

StaticArray<DOTS::comp_info,32> DOTS::rtti;
DOTS::Register *reg;
DOTS::ThreadPool *tp;


void f1(DOTS::Entity, int& v1){
    v1 = 69;
    printf("%p\n",&v1);
}

void f2(DOTS::Entity, int v1){
    printf("%d\n",v1);
}

struct Hierarchy {
    DOTS::Entity parent;
    SmallVector<DOTS::Entity,8> child;
};
struct LocalTransform {
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);
    glm::quat rotation = glm::quat(glm::cos(glm::radians(90.0f/2)), 0.0, glm::sin(glm::radians(90.0f / 2)), 0.0);
};
typedef glm::mat<4, 4, float, glm::precision::defaultp> GlobalTransform;

class TransformJob : public DOTS::Job {
    DOTS::entity_range next(DOTS::entity_t f){
        return reg->findChunk<Hierarchy>(f,5);
    }
    void proc(DOTS::entity_range es){
        if(es.end > es.begin){
            const size_t count = es.end - es.begin;
            Hierarchy *ptr = reg->getComponent2<Hierarchy>(DOTS::entity_t{.index=es.begin,.archetype=es.archetype});
            printf("{");
            for (size_t i = 0; i < count; i++)
                printf("%u: %u, ",es.begin+i,(uint32_t)ptr[i].parent);
            printf("}\n");
        }
    }
};
static glm::mat4 pissofshit = glm::mat4{0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5};

class StressJob : public DOTS::Job {
    DOTS::entity_range next(DOTS::entity_t f){
        return DOTS::entity_range{.begin=f.index, .end= f.index<16?f.index+1:f.index,.archetype=DOTS::null_archetype_index};
    }
    void proc(DOTS::entity_range es){
        glm::mat4 useless = pissofshit;
        for (size_t i = 0; i < 0xffffff; i++)
            useless *= pissofshit;
        pissofshit=useless;
    }
};

class TransformSystem : public DOTS::System {
    void update(){
        tp->addJob(new StressJob(),0);
        //tp->addJob(new TransformJob(),0);
    }
public:
    TransformSystem(){
    }
};

int main(){
    {
        bitset b1;
        bitset b2;
        b1.set(4);
        b2.set(7);
        //b.resize(10);
        printf("res: %d\n",b1.and_equal(b2));

    }
    reg = new DOTS::Register();
    tp = new DOTS::ThreadPool(8);
    auto v0 = reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0);
    auto v1 = reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0);
    reg->addComponents(v1,DOTS::componentsBitmask<bool>());
    auto v2 = reg->create<int>(0);
    auto v3 = reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0);
    auto v4 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    reg->removeComponents(v1,DOTS::componentsBitmask<bool>());
    auto v5 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});

    reg->iterate(f1);
    reg->iterate<int>([](std::array<void*,2> arg,size_t chunk_size){
        printf("{");
        for (size_t i = 0; i < chunk_size; i++)
        {
            printf("%d, ",((int*)arg[0])[i]);
        }
        printf("}\n");
    });
    reg->iterate<int>(f2);

    delete tp;
    delete reg;
    printf("done\n");
}
