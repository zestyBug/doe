#include "cutil/basics.hpp"
#include "ECS/Base/TypeID.hpp"
#include "ECS/Base/Entity.hpp"
#include "cutil/HashHelper.hpp"

namespace ECS {

uint32_t              TypeManager::typeCount = 2;
TypeManager::TypeInfo TypeManager::sharedTypeInfos[TypeManager::MaximumTypesCount];
const char*           TypeManager::sharedTypeNames[TypeManager::MaximumTypesCount];

void TypeID::Debug() {
    const TypeManager::TypeInfo& info = TypeManager::GetTypeInfo(*this);
    const char* name = TypeManager::GetTypeName(*this);
    printf("Type: %s, index: %u, size: %u\n",name, info.TypeIndex.index(), info.TypeSize);
    printf("\tCategory: %s\n", 
        info.Category == TypeManager::TypeCategory::Entity ? "Entity" : 
        info.Category == TypeManager::TypeCategory::IComponentData ? "IComponentData" : 
        info.Category == TypeManager::TypeCategory::ISharedComponentData ? "ISharedComponentData" : 
        "Unknown");
    printf("\tisSharedComponent: %i\n", this->isSharedComponent());
    printf("\tisZeroSized: %i\n", this->isZeroSized());
    printf("\tisManaged: %i\n", this->isManaged());
}

template<> TypeID __typeid__<nullptr_t>(){
    static TypeID id = TypeManager::registerNull();
    return id;
}
template<> TypeID __typeid__<ECS::Entity>(){
    static TypeID id = TypeManager::registerEntity();
    return id;
}

}
ssize_t allocator_counter = 0;