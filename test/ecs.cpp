//#include "Engine/ECS/ThreadPool.hpp"
#include "ECS/Register.hpp"
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
    DOTS::Entity parent{};
    SmallVector<DOTS::Entity,8> child{};
};
struct LocalTransform {
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);
    glm::quat rotation = glm::quat(glm::cos(glm::radians(90.0f/2)), 0.0, glm::sin(glm::radians(90.0f / 2)), 0.0);
};
typedef glm::mat<4, 4, float, glm::precision::defaultp> GlobalTransform;

int main(){
    reg = new DOTS::Register();
    DOTS::getTypeInfo<DOTS::Entity>();
    auto v0 = reg->createEntity<Hierarchy,GlobalTransform,int,prototype>();
    auto v1 = reg->createEntity<Hierarchy,GlobalTransform,int,prototype>();
    auto v2 = reg->createEntity<Hierarchy,GlobalTransform,int,prototype>();
    reg->createEntity<Hierarchy,GlobalTransform,int,prototype>();
    reg->createEntity<GlobalTransform,int,prototype>();
    auto v3 = reg->createEntity<GlobalTransform,int,prototype>();
    reg->createEntity<GlobalTransform,int,prototype>();
    reg->destroyEntity(v1);
    reg->destroyEntity(v0);
    reg->destroyEntity(v2);
    reg->destroyEntity(v3);
    delete reg;
}
