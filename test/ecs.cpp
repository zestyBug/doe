#include "src/Engine/ThreadPool.hpp"
#include "src/Engine/Register.hpp"
#include "src/Engine/System.hpp"
#include "cutil/Range.hpp"

StaticArray<DOTS::comp_info,32> DOTS::rtti;
DOTS::Register *reg;
DOTS::ThreadPool *tp;


void f1(DOTS::Entity, int& v1){
    v1 = 69;
}

void f2(DOTS::Entity, int v1){
    printf("%d\n",v1);
}

class TransformSystem : public DOTS::System {
    void update(){
        tp->addJob(DOTS::Job{ 
            [](DOTS::entity_t f){
                return reg->findChunk<int>(f,5);
            }, [](DOTS::entity_range es){
                if(es.end > es.begin){
                    const DOTS::entity_t count = es.end - es.begin;
                    int *ptr = reg->getComponent2<int>(es.begin);
                    printf("{");
                    for (DOTS::entity_t i = 0; i < count; i++)
                    {
                        printf("%u: %d, ",es.begin+i,ptr[i]);
                    }
                    printf("}\n");
                }
            }
        },0);
    }
public:
    TransformSystem(){
    }
};


int main(){
    reg = new DOTS::Register();
    tp = new DOTS::ThreadPool(1);
    auto v0 = reg->create<int,float>();
    auto v1 = reg->create<int,float,bool>();
    auto v2 = reg->create<int,float,bool>();
    auto v3 = reg->create<int,float>();
    auto v4 = reg->create<float,int>();
    reg->getComponent<int>(v0) = 10;
    reg->getComponent<int>(v1) = 11;
    reg->getComponent<int>(v2) = 12;
    reg->getComponent<int>(v3) = 13;
    reg->getComponent<int>(v4) = 14;
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
    reg->addSystem<TransformSystem>();
    reg->executeSystems();
    tp->restart();
    tp->wait();

    delete tp;
    delete reg;
    printf("done\n");
}