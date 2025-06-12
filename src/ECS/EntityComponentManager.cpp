#include "ECS/EntityComponentManager.hpp"
using namespace ECS;




Archetype* EntityComponentManager::getOrCreateArchetype(const_span<TypeID> types){
#ifdef DEBUG
    if(types.size() < 2)
        throw std::invalid_argument("getOrCreateArchetype(): Archetype must contain atleast 2 component");
#endif
    if(types[0].value != 0)
        throw std::invalid_argument("getOrCreateArchetype(): types must contain the Entity component");
    {
        Archetype* archetype = archetypeTypeMap.tryGet(types);
        if(archetype != nullptr)
            // assume archetype is already initialized and we dont store destroyed archtypes here.
            return archetype;
    }
    {
        // unique_ptr manages deallocation on exceptions
        ArchetypeHolder archetype_ptr{Archetype::createArchetype(types)};
        Archetype * const archetype = archetype_ptr.get();

        if(!archetype)
            throw std::runtime_error("getOrCreateArchetype(): createArchetype() failed");

        archetypeTypeMap.add(archetype);

        if(FreeArchetypeIndex >= NullArchetypeIndex){
            uint32_t archetypeIndex = (uint32_t) archetypes.size();
            if((archetypeIndex+1) >= NullArchetypeIndex)
                throw std::out_of_range("archtype index full");
            archetype->archetypeIndex = archetypeIndex;
            archetypes.push_back(std::move(archetype_ptr));
        }else{
            ArchetypeHolder& ptr = archetypes.at(FreeArchetypeIndex);
            /// TODO: may a invalid index check be good
            if(( (intptr_t)ptr.get_raw() & 0x1) != 1)
                throw std::runtime_error("getOrCreateArchetype(): unexpected free archetype");
            archetype->archetypeIndex = FreeArchetypeIndex;
            FreeArchetypeIndex = (uint32_t)((intptr_t)ptr.get_raw() >> 1);
            ptr = std::move(archetype_ptr);
        }
        return archetype;
    }
}
void EntityComponentManager::destroyEmptyArchetype(const uint32_t archetypeIndex){
    // assuming archetypeIndex is less that NullArchetypeIndex
    Archetype * arch = this->archetypes[archetypeIndex].get();
    if(arch && arch->empty()) {
        //printf("Debug: destroy archetype bitmask %u\n",this->archetypes_id[archetype_index]);
        this->archetypes[archetypeIndex].reset((Archetype*)( (intptr_t)(FreeArchetypeIndex << 1)&1 ) );
        FreeArchetypeIndex = archetypeIndex;
    }
}
Archetype *EntityComponentManager::getArchetypeWithAddedComponents(Archetype *archetype,const_span<TypeID> componentTypeSet) noexcept {
    const_span<TypeID> srcTypes = archetype->types;
    uint32_t dstTypesCount = srcTypes.size() + componentTypeSet.size();

    TypeID dstTypes[dstTypesCount];

    // zipper the two sorted arrays
    // because this is done in-place,
    // it must be done backwards so as not to disturb the existing contents.

    uint32_t unusedIndices = 0;
    {
        int32_t oldThings = (int32_t)srcTypes.size() - 1;
        int32_t newThings = (int32_t)componentTypeSet.size() - 1;
        uint32_t mixedThings = dstTypesCount;
        while (newThings >= 0) // oldThings[0] has value 0, newThings can't have anything lower than that
        {
            // there is no way oldThings be less than 0 if things goes currectly
            // type arrays being sorted and getTypeInfo<TypeID>().value.value() == 0
            TypeID oldThing = srcTypes[oldThings];
            TypeID newThing = componentTypeSet[newThings];
            if (oldThing.value > newThing.value) // put whichever is bigger at the end of the array
            {
                dstTypes[--mixedThings] = oldThing;
                --oldThings;
            }
            else
            {
                if (oldThing.value == newThing.value)
                    --oldThings;
                // note: may values be same but newThing amy contain diffrent value
                // so newThing has higher perioriry
                dstTypes[--mixedThings] = newThing;
                --newThings;
            }
        }
        while (oldThings >= 0) // if there are remaining old things, copy them here
        {
            dstTypes[--mixedThings] = srcTypes[oldThings--];
        }
        unusedIndices = mixedThings; // In case we ignored duplicated types, this will be > 0
    }

    // if it is here means:
    if (unusedIndices == componentTypeSet.size())
        return archetype;

    return this->getOrCreateArchetype({dstTypes,dstTypesCount});
}
Archetype* EntityComponentManager::getArchetypeWithAddedComponent(Archetype* archetype, TypeID componentType,uint32_t *indexInTypeArray) noexcept {
    const_span<TypeID> srcTypes = archetype->types;
    uint32_t dstTypesCount = (uint32_t)srcTypes.size() + 1;

    TypeID newTypes[dstTypesCount];

    uint16_t t = 0;
    while (t < srcTypes.size() && srcTypes[t].value < componentType.value)
    {
        newTypes[t] = srcTypes[t];
        ++t;
    }

    if (indexInTypeArray != nullptr)
        *indexInTypeArray = t;

    if (t != srcTypes.size() && srcTypes[t].exactSame(componentType)) {
        // Tag component type is already there, no new archetype required.
        return archetype;
    }
    newTypes[t] = componentType;

    while (t < srcTypes.size())
    {
        newTypes[t + 1] = srcTypes[t];
        ++t;
    }

    return getOrCreateArchetype({newTypes,dstTypesCount});
}
Archetype* EntityComponentManager::getArchetypeWithRemovedComponents(Archetype *archetype,const_span<TypeID> typeSetToRemove) noexcept {
    const uint32_t typesCount = archetype->types.size();
    TypeID newTypes[typesCount];

    uint32_t numRemovedTypes = 0;
    for (uint32_t t = 0; t < typesCount; ++t)
    {
        const uint16_t existingTypeIndex = archetype->types[t].value;

        bool removed = false;
        for(const TypeID typeMember : typeSetToRemove){
            const uint16_t type = getTypeInfo(typeMember).value.value;
            if (existingTypeIndex == type)
            {
                numRemovedTypes++;
                removed = true;
                break;
            }
        }

        if (!removed)
            newTypes[t - numRemovedTypes] = archetype->types[t];
    }

    if (numRemovedTypes == 0)
        return archetype;
    /// TODO: will behave unexpected if 'Entity' is removed!
    const_span<TypeID> args{newTypes, typesCount - numRemovedTypes};
    // if anything other than Entity is ramaind
    if(args.size() > 1)
        return getOrCreateArchetype(args);
    else
        return nullptr;
}
Archetype* EntityComponentManager::getArchetypeWithRemovedComponent(Archetype* archetype,TypeID removedComponentType,uint32_t *indexInOldTypeArray) noexcept {
    const uint32_t typesCount = archetype->types.size();
    TypeID newTypes[typesCount];

    uint32_t removedTypes = 0;
    for (uint32_t t = 0; t < typesCount; ++t)
        if (archetype->types[t].value == removedComponentType.value)
        {
            if (indexInOldTypeArray != nullptr)
                *indexInOldTypeArray = t;
            ++removedTypes;
        }
        else
            newTypes[t - removedTypes] = archetype->types[t];

    /// TODO: will behave unexpected if 'Entity' is removed!
    const_span<TypeID> args{newTypes, typesCount - removedTypes};
    if(args.size() > 1)
        return getOrCreateArchetype(args);
    else
        return nullptr;
}
Archetype* EntityComponentManager::getArchetype(Entity entity) {
    const entity_t& srcValue = validate(entity);
    return srcValue.archetype < this->archetypes.size() ? 
        this->archetypes[srcValue.archetype].get() :
        nullptr;
}



















const Archetype* EntityComponentManager::getArchetype(Entity entity) const {
    const entity_t& srcValue = validate(entity);
    return srcValue.archetype < this->archetypes.size() ? 
        this->archetypes[srcValue.archetype].get() :
        nullptr;
}
Entity EntityComponentManager::createEntity(){
    return recycleEntity();
}
void EntityComponentManager::addComponent(Entity entity, TypeID component){
    entity_t& srcValue = validate(entity);

    Archetype *srcArchetype,*newArchetype;

    if(srcValue.validArchtype()){
        //entity_value must contain valid value, any exception is our fault and flaw
        srcArchetype = this->archetypes.at(srcValue.archetype).get();
        newArchetype = this->getArchetypeWithAddedComponent(srcArchetype, component);
        if(srcArchetype == newArchetype)
            return;
        Helper_moveEntityToNewArchetype(newArchetype,srcArchetype, &srcValue);
    }else{
        Helper_allocateInArchetype({&component,1}, &srcValue, entity);
    }
}
void EntityComponentManager::addComponents(Entity entity, const_span<TypeID> componentTypeSet)
{
    entity_t& srcValue = validate(entity);

    Archetype *srcArchetype,*newArchetype;

    if(srcValue.validArchtype()){
        //entity_value must contain valid value, any exception is our fault and flaw
        srcArchetype = this->archetypes.at(srcValue.archetype).get();
        newArchetype = this->getArchetypeWithAddedComponents(srcArchetype, componentTypeSet);
        if(srcArchetype == newArchetype)
            return;
        Helper_moveEntityToNewArchetype(newArchetype,srcArchetype, &srcValue);
    }else
        Helper_allocateInArchetype(componentTypeSet, &srcValue, entity);
}
void EntityComponentManager::removeComponent(Entity entity, TypeID component){
    entity_t& srcValue = validate(entity);

    Archetype *srcArchetype,*newArchetype;

    if(srcValue.validArchtype()){
        //entity_value must contain valid value, any exception is our fault and flaw
        srcArchetype = this->archetypes.at(srcValue.archetype).get();
        newArchetype = this->getArchetypeWithRemovedComponent(srcArchetype, component);
        if (srcArchetype == newArchetype)  // none were added
            return;
        if(newArchetype)
            Helper_moveEntityToNewArchetype(newArchetype,srcArchetype, &srcValue);
        else{
            Helper_removeFromArchetype(srcArchetype,&srcValue);
        }
    }
}
void EntityComponentManager::removeComponents(Entity entity, const_span<TypeID> componentTypeSet){
    entity_t& srcValue = validate(entity);

    Archetype *srcArchetype,*newArchetype;

    if(srcValue.validArchtype()){
        //entity_value must contain valid value, any exception is our fault and flaw
        srcArchetype = this->archetypes.at(srcValue.archetype).get();
        newArchetype = this->getArchetypeWithRemovedComponents(srcArchetype, componentTypeSet);
        if (srcArchetype == newArchetype)  // none were removed
            return;
        if(newArchetype)
            Helper_moveEntityToNewArchetype(newArchetype,srcArchetype, &srcValue);
        else{
            Helper_removeFromArchetype(srcArchetype,&srcValue);
        }
    }
}
Entity EntityComponentManager::createEntity(const_span<TypeID> types) {
    const Entity result = recycleEntity();
    if(!types.empty())
    {
        entity_t &value = this->entity_value[result.index()];
        Helper_allocateInArchetype(types,&value,result);
    }
    return result;
}
void EntityComponentManager::destroyEntity(Entity entity) {
    validate(entity);

    removeComponents(entity);
    recycleEntity(entity);
}
void EntityComponentManager::removeComponents(Entity entity)
{
    entity_t& value = validate(entity);

    // value.validArchtype() isnt enough
    if(value.validArchtype()){
        Archetype *arch = this->archetypes.at(value.archetype).get();
        if(!arch)
            throw std::runtime_error("destroyEntity(): entity points to invalid archetype");
        Helper_removeFromArchetype(arch,&value);
    }
}

void* EntityComponentManager::getComponent(Entity entity,TypeID type){
    const entity_t& value = validate(entity);
    if(value.validArchtype()){
        Archetype *arch = this->archetypes.at(value.archetype).get();
        if(!arch)
            throw std::runtime_error("destroyEntity(): entity points to invalid archetype");
        const int tindex = arch->getIndex(type);
        if((tindex & 0xFFFF) == tindex)
            return arch->getComponent(tindex,value.index,globalVersion);
    }
    return nullptr;
}
const void* EntityComponentManager::getComponent(Entity entity,TypeID type) const {
    const entity_t& value = validate(entity);
    if(value.validArchtype()){
        Archetype *arch = this->archetypes.at(value.archetype).get();
        if(!arch)
            throw std::runtime_error("destroyEntity(): entity points to invalid archetype");
        const int tindex = arch->getIndex(type);
        if((tindex & 0xFFFF) == tindex)
            return arch->getComponent(tindex,value.index);
    }
    return nullptr;
}
bool EntityComponentManager::hasComponents(Entity entity,const_span<TypeID> types) const {
    const entity_t& value = validate(entity);

    if(types.size() < 1)
        return true;
    if(value.archetype > this->archetypes.size())
        return false;
    const Archetype* arch = this->archetypes.at(value.archetype).get();
    return arch->hasComponents(types);
}
bool EntityComponentManager::hasComponent(Entity entity,TypeID type) const {
    const entity_t& value = validate(entity);
    if(value.archetype > this->archetypes.size())
        return false;
    const Archetype *arch = this->archetypes.at(value.archetype).get();
    return arch->hasComponent(type);
}
bool EntityComponentManager::hasArchetype(Entity entity) const {
    const entity_t& value = validate(entity);
    return value.archetype < this->archetypes.size();
}












void EntityComponentManager::Helper_allocateInArchetype(Archetype *newArchetype,entity_t *srcValue, Entity entity) noexcept {
    const uint32_t newIndex = newArchetype->createEntity(globalVersion);
    newArchetype->callComponentConstructor(newIndex,entity);
    srcValue->index = newIndex;
    srcValue->archetype = newArchetype->archetypeIndex;
}
void EntityComponentManager::Helper_allocateInArchetype(const_span<TypeID> componentTypeSet,entity_t *srcValue, Entity entity) noexcept {
    TypeID componentTypeSetBuffer[1 + componentTypeSet.size()];
    componentTypeSetBuffer[0] = getTypeInfo<Entity>().value;
    memcpy(componentTypeSetBuffer+1,componentTypeSet.data(),componentTypeSet.size_bytes());
    Archetype *newArchetype = this->getOrCreateArchetype({componentTypeSetBuffer,1 + componentTypeSet.size()});
    const uint32_t newIndex = newArchetype->createEntity(globalVersion);
    newArchetype->callComponentConstructor(newIndex,entity);
    srcValue->index = newIndex;
    srcValue->archetype = newArchetype->archetypeIndex;
} 
void EntityComponentManager::Helper_removeFromArchetype(Archetype *srcArchetype,entity_t *srcValue) noexcept {
    Entity replacedEntity = Archetype::managedRemoveEntity(srcArchetype,srcValue->index);
    // if it index was filled with other entity
    if(replacedEntity.valid())
            this->entity_value.at(replacedEntity.index()).index = srcValue->index;
    srcValue->archetype = NullArchetypeIndex;
}
void EntityComponentManager::Helper_moveEntityToNewArchetype(Archetype *__restrict__ newArchetype,Archetype *__restrict__ srcArchetype, entity_t *srcValue) noexcept {
    // see addComponent(s)
#ifdef DEBUG
    if (srcArchetype == newArchetype)
        throw std::invalid_argument("Helper_moveEntityToNewArchetype(): unexpected same archetype");
#endif
    const uint32_t srcIndex = srcValue->index;
    const uint32_t newIndex = newArchetype->createEntity(globalVersion);

    Archetype::moveComponentValues(newArchetype,srcArchetype,newIndex,srcIndex);

    const uint32_t replacedIndex = srcArchetype->removeEntity(srcIndex);
    // if it index was filled with other entity
    if(replacedIndex!=srcIndex)
    {
        const Entity replacedEntity = Archetype::copyComponentValues(srcArchetype,srcIndex,replacedIndex);
        this->entity_value.at(replacedEntity.index()).index = srcIndex;
    }

    srcValue->index = newIndex;
    srcValue->archetype = newArchetype->archetypeIndex;
}


/*

        template<typename TypeID>
        inline TypeID chunk_wrapper(const size_t chunk_index, const std::array<size_t,COMPOMEN_COUNT>& offsets, void*chunk){
            getTypeInfo<TypeID>().ko;
            return (TypeID*)(((char*)chunk) + offsets[type_id<TypeID>().index])[chunk_index];
        }

        template<typename TypeID>
        inline void init_wrapper(uint32_t index, const std::vector<std::pair<typeid_t,size_t>>& offsets, void*chunk, const TypeID& init_value){
            for(const auto& offset:offsets)
                if(offset.first == type_id<TypeID>().index)
                    new (((TypeID*)(((uint8_t*)chunk) + offset.second))+index) TypeID(init_value);
        }
}*/


void EntityComponentManager::recycleEntity(Entity entity){
    entity_t& value = this->entity_value.at(entity.index());
    value.index = (uint32_t)FreeEntityIndex;
    value.archetype = NullArchetypeIndex;
    // increasing version prevent from double destroing objects
    value.version++;
    FreeEntityIndex = entity.index();
}
Entity EntityComponentManager::recycleEntity(){
    Entity result;
    if(FreeEntityIndex >= 0){
        entity_t& value = this->entity_value.at(FreeEntityIndex);
        result = Entity{FreeEntityIndex,value.version};
        // version and archetype mus be set on destroyEntity
        FreeEntityIndex = (int32_t)value.index;
    }else{
        const size_t index = entity_value.size();
        if(index > INT32_MAX)
            throw std::out_of_range("createEntity(): entity count limit reached");
        entity_value.emplace_back();
        result = Entity{(int32_t)index,0};
    }
    return result;
}