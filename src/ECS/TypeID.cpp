#include "cutil/basics.hpp"
#include "ECS/Base/TypeID.hpp"
#include "ECS/Base/Entity.hpp"
#include "cutil/HashHelper.hpp"
#include "ECS/Base/Constants.hpp"

namespace ECS {

uint32_t              TypeManager::typeCount = 2;
TypeManager::TypeInfo TypeManager::sharedTypeInfos[Constants::MaximumTypesCount];
const char*           TypeManager::sharedTypeNames[Constants::MaximumTypesCount];

TypeID TypeManager::registerNull() {
    // MAGIC NUMBER
    sharedTypeInfos[0].TypeIndex = TypeID::fromIndex(ZeroSizeInChunkTypeFlag);
    sharedTypeInfos[0].TypeSize = sizeof(nullptr_t);
    sharedTypeInfos[0].SizeInChunk = 0;
    sharedTypeInfos[0].AlignmentInBytes = alignof(nullptr_t);
    sharedTypeInfos[0].Category = TypeCategory::IComponentData;
    sharedTypeNames[0]="nullptr_t";
    return TypeID::fromIndex(ZeroSizeInChunkTypeFlag);
}
TypeID TypeManager::registerEntity() {
    static_assert(sizeof(Entity) == 8);
    // MAGIC NUMBER
    sharedTypeInfos[1].TypeIndex = TypeID::fromIndex(1);
    sharedTypeInfos[1].TypeSize = sizeof(Entity);
    sharedTypeInfos[1].SizeInChunk = sizeof(Entity);
    sharedTypeInfos[1].AlignmentInBytes = alignof(Entity);
    sharedTypeInfos[1].Category = TypeCategory::Entity;
    sharedTypeNames[1]="Entity";
    return TypeID::fromIndex(1);
}

void TypeID::Debug() {
    const TypeManager::TypeInfo& info = TypeManager::GetTypeInfo(*this);
    const char* name = TypeManager::GetTypeName(*this);
    printf("Type: %s, index: %u, size: %u, size in chunk: %u\n",name, info.TypeIndex.index(), info.TypeSize, info.SizeInChunk);
    printf("\tCategory: %s\n", 
        info.Category == TypeManager::TypeCategory::Entity ? "Entity" : 
        info.Category == TypeManager::TypeCategory::IComponentData ? "IComponentData" : 
        info.Category == TypeManager::TypeCategory::ISharedComponentData ? "ISharedComponentData" : 
        "Unknown");
    printf("\tisSharedComponent: %i\n", this->isSharedComponent());
    printf("\tisZeroSized: %i\n", this->isZeroSized());
    printf("\tisManaged: %i\n", this->isManagedComponent());
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