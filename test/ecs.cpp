//#include "Engine/ECS/ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "cutil/range.hpp"
#include "cutil/SmallVector.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "cutil/prototype.hpp"
ECS::EntityComponentManager *reg;
ssize_t allocator_counter = 0;
int prototype::counter = 0;

//ECS::ThreadPool *tp;

struct Hierarchy {
    ECS::Entity parent{};
    SmallVector<ECS::Entity,8> child{};
};
struct LocalTransform {
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);
    glm::quat rotation = glm::quat(glm::cos(glm::radians(90.0f/2)), 0.0, glm::sin(glm::radians(90.0f / 2)), 0.0);
};
typedef glm::mat<4, 4, float, glm::precision::defaultp> GlobalTransform;

int main(){
    reg = new ECS::EntityComponentManager();
    ECS::getTypeInfo<ECS::Entity>();

    {
        ECS::ArchetypeVersionManager chunks;
        chunks.initialize(4);
        for (size_t i = 0; i < 100; i++)
        {
            chunks.add(0);
        }
        chunks.popBack();
    }

    auto v0 = reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    reg->destroyEntity(v0);
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    }
    auto v1 = reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    reg->destroyEntity(v1);
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    }
    auto v2 = reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    for (size_t i = 0; i < 100; i++)
    {
        reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    }
    auto v4 = reg->createEntity(ECS::componentTypes<Hierarchy,GlobalTransform,int>());
    
    
    reg->destroyEntity(v4);
    reg->destroyEntity(v2);
    
    reg->iterate<int>([](span<void*>,uint32_t count){
        printf("count: %u\n",count);
    });

    delete reg;
    printf("counter: %li\n",allocator_counter);
}
