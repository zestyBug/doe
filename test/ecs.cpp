#include "src/Engine/ThreadPool.hpp"
#include "src/Engine/Register.hpp"
#include "src/Engine/System.hpp"
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
            Hierarchy *ptr = reg->getComponent2<Hierarchy>(DOTS::entity_t{.index=es.begin,.archtype=es.archtype});
            printf("{");
            for (size_t i = 0; i < count; i++)
                printf("%u: %d, ",es.begin+i,ptr[i].parent);
            printf("}\n");
        }
    }
};

class TransformSystem : public DOTS::System {
    void update(){
        tp->addJob(new TransformJob(),0);
    }
public:
    TransformSystem(){
    }
};

int main(){
    reg = new DOTS::Register();
    tp = new DOTS::ThreadPool(1);
    auto v0 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    auto v1 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    auto v2 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    auto v3 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    auto v4 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    auto v5 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    // reg->addComponent<bool>(v1);
    // reg->removeComponent<bool>(v1);
   
    /*reg->iterate<int&>(f1);
    reg->iterate<int>([](std::array<void*,2> arg,size_t chunk_size){
        printf("{");
        for (size_t i = 0; i < chunk_size; i++)
        {
            printf("%d, ",((int*)arg[0])[i]);
        }
        printf("}\n");
    });
    reg->iterate<int>(f2);*/
    
   

    tp->wait();
    reg->addSystem(new TransformSystem());
    reg->executeSystems();
    tp->restart();
    tp->wait();

    delete tp;
    delete reg;
    printf("done\n");
}