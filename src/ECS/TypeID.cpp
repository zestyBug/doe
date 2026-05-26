#include "cutil/basics.hpp"
#include "ECS/Base/TypeID.hpp"
#include "ECS/Base/Entity.hpp"
#include "cutil/HashHelper.hpp"
#include "ECS/Base/Constants.hpp"


namespace ECS {

uint32_t              TypeManager::initialized = 0;
uint32_t              TypeManager::assetRefOffsetIndex = 0;
uint32_t              TypeManager::typeCount = 2;
align_ptr<TypeManager::TypeInfo[]> TypeManager::sharedTypeInfos = nullptr;
align_ptr<uint16_t[]>              TypeManager::assetRefOffsetList = nullptr;


void TypeManager::Initialize(){
    TypeManager::initialized = 1;
    TypeManager::sharedTypeInfos    = make_align<TypeManager::TypeInfo[]>(Constants::MaximumTypesCount);
    TypeManager::assetRefOffsetList = make_align<uint16_t[]>(Constants::MaximumRefOffsetCount);
    
    // MAGIC NUMBER
    sharedTypeInfos[0].TypeIndex = TypeID::fromIndex(ZeroSizeInChunkTypeFlag);
    sharedTypeInfos[0].TypeSize = sizeof(std::nullptr_t);
    sharedTypeInfos[0].SizeInChunk = 0;
    sharedTypeInfos[0].AlignmentInBytes = alignof(std::nullptr_t);
    sharedTypeInfos[0].Category = TypeCategory::IComponentData;
    sharedTypeInfos[0].Name = "std::nullptr_t";

    static_assert(sizeof(Entity) == 8);
    // MAGIC NUMBER
    sharedTypeInfos[1].TypeIndex = TypeID::fromIndex(1);
    sharedTypeInfos[1].TypeSize = sizeof(Entity);
    sharedTypeInfos[1].SizeInChunk = sizeof(Entity);
    sharedTypeInfos[1].AlignmentInBytes = alignof(Entity);
    sharedTypeInfos[1].Category = TypeCategory::Entity;
    sharedTypeInfos[1].Name = "ECS::Entity";
}

void TypeID::Debug() {
    const TypeManager::TypeInfo& info = TypeManager::GetTypeInfo(*this);
    const char* name = TypeManager::GetTypeName(*this);
    printf("Type: %s, index: %u, size: %u, size in chunk: %u\n"
                "\tCategory: %s\n"
                "\tisSharedComponent: %i\n"
                "\tisZeroSized: %i\n"
                "\tisManaged: %i\n"
        ,name, info.TypeIndex.index(), info.TypeSize, info.SizeInChunk,
        info.Category == TypeManager::TypeCategory::Entity ? "Entity" : 
         info.Category == TypeManager::TypeCategory::IComponentData ? "IComponentData" : 
          info.Category == TypeManager::TypeCategory::ISharedComponentData ? "ISharedComponentData" : "Unknown",
        this->isSharedComponent(),
        this->isZeroSized(),
        this->isManagedComponent());
}

template<> TypeID __typeid__<std::nullptr_t>(){
    return TypeID::fromIndex(0);
}
template<> TypeID __typeid__<ECS::Entity>(){
    return TypeID::fromIndex(1);
}

}
ssize_t allocator_counter = 0;