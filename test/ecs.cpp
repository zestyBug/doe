//#include "Engine/ECS/ThreadPool.hpp"
#include "Engine/ECS/Register.hpp"
#include "Engine/ECS/System.hpp"
#include "cutil/range.hpp"
#include "cutil/SmallVector.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cutil/prototype.hpp"
StaticArray<DOTS::comp_info,32> DOTS::rtti;
DOTS::Register *reg;
//DOTS::ThreadPool *tp;

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

// class TransformJob : public DOTS::Job {
//     DOTS::entity_range next(DOTS::entity_t f){
//         return reg->findChunk<Hierarchy>(f,5);
//     }
//     void proc(DOTS::entity_range es){
//         if(es.end > es.begin){
//             const size_t count = es.end - es.begin;
//             Hierarchy *ptr = reg->getComponent2<Hierarchy>(DOTS::entity_t{.index=es.begin,.archetype=es.archetype});
//             printf("{");
//             for (size_t i = 0; i < count; i++)
//                 printf("%u: %u, ",es.begin+i,(uint32_t)ptr[i].parent);
//             printf("}\n");
//         }
//     }
// };
static glm::mat4 pissofshit = glm::mat4{0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5};

// class StressJob : public DOTS::Job {
//     DOTS::entity_range next(DOTS::entity_t f){
//         return DOTS::entity_range{.begin=f.index, .end= f.index<16?f.index+1:f.index,.archetype=DOTS::null_archetype_index};
//     }
//     void proc(DOTS::entity_range es){
//         glm::mat4 useless = pissofshit;
//         for (size_t i = 0; i < 0xffffff; i++)
//             useless *= pissofshit;
//         pissofshit=useless;
//     }
// };


class TransformSystem : public DOTS::System {
    void update(){
        //tp->addJob(new StressJob(),0);
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
    //tp = new DOTS::ThreadPool(8);
    auto v0 = reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0);
    //reg->destroy(v0);

    size_t rand_seed = 1;

    for(size_t j=0;j<2;j++){
        std::vector<DOTS::Entity> buffer;
        buffer.reserve(512);

        for(size_t i=0;i<64;i++){
            rand_seed = (rand_seed ^ 0xDeadBee0BedBeaf) * 0x17 + 0xBADFEED;
            uint8_t buff = rand_seed & 0xff;
            if(buff&0x111)
                buffer.emplace_back(reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0));
            else if(buff&0x11)
                buffer.emplace_back(reg->create<Hierarchy,float>(Hierarchy{},0.0f));
            else
                buffer.emplace_back(reg->create<Hierarchy>(Hierarchy{}));
        }

        for(size_t i=0;i<32;i++){
            rand_seed = (rand_seed ^ 0xDeadBee0BedBeaf) * 0x17 + 0xBADFEED;
            uint8_t buff = rand_seed & 0xff;
            if(buff&0x111)
                buffer.emplace_back(reg->create<Hierarchy,float>(Hierarchy{},0.0f));
            else if(buff&0x1)
                buffer.emplace_back(reg->create<Hierarchy>(Hierarchy{}));
            else{
                reg->destroy(buffer.back());
                buffer.pop_back();
            }
        }

        for(size_t i=0;i<32;i++){
            //rand_seed = (rand_seed ^ 0xDeadBee0BedBeaf) * 0x17 + 0xBADFEED;
            //uint8_t buff = rand_seed & 0xff;
            // if(buff&0x111)
            //     reg->addComponents(buffer.at(i),DOTS::componentsBitmask<bool>());
            // if(buff&0x1)
            //     reg->removeComponents(buffer.at(i),DOTS::componentsBitmask<Hierarchy>());
        }

        for(const auto v:buffer){
            reg->destroy(v);
        }
    }
    
    v0 = reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0);
    auto v1 = reg->create<GlobalTransform,Hierarchy>(GlobalTransform{},Hierarchy{});
    reg->addComponents<int>(v1,0);
    auto v2 = reg->create<int>(0);
    auto v3 = reg->create<Hierarchy,GlobalTransform,int>(Hierarchy{},GlobalTransform{},0);
    auto v4 = reg->create<Hierarchy,GlobalTransform>(Hierarchy{},GlobalTransform{});
    reg->removeComponents<bool>(v1);
    auto v5 = reg->create<Hierarchy,GlobalTransform,prototype>(Hierarchy{},GlobalTransform{},prototype{});
    //reg->destroy(v5);

    reg->iterate<int>([](std::array<void*,1> arg,DOTS::Entity*,size_t chunk_size){
        for (size_t i = 0; i < chunk_size; i++)
        {
            ((int*)arg[0])[i] = 10;
        }
    });


    reg->iterate<int>([](std::array<void*,1> arg,DOTS::Entity*,size_t chunk_size){
        printf("{");
        for (size_t i = 0; i < chunk_size; i++)
        {
            printf("%d, ",((int*)arg[0])[i]);
        }
        printf("}\n");
    });

    reg->iterate<prototype>([](std::array<void*,1> arg,DOTS::Entity*,size_t chunk_size){
        for (size_t i = 0; i < chunk_size; i++)
        {
            printf("prototype: %p\n",(prototype*)arg[0]+i);
        }
    });


    //delete tp;
    delete reg;
    //printf("done\n");
}
