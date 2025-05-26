#include "ECS/EntityComponentManager.hpp"
using namespace ECS;




Archetype* EntityComponentManager::getOrCreateArchetype(span<TypeIndex> types){
    {
        Archetype* archetype = archetypeTypeMap.tryGet(types);
        if(archetype != nullptr)
            // assume archetype is already initialized and we dont store destroyed archtypes here.
            return archetype;
    }
    {
        // unique_ptr manages deallocation on exceptions
        unique_ptr<Archetype> archetype_ptr{Archetype::createArchetype(types)};
        Archetype * const archetype = archetype_ptr.get();

        if(!archetype)
            throw std::runtime_error("getOrCreateArchetype(): createArchetype() failed");

        archetypeTypeMap.add(archetype);

        if(freeArchetypeIndex >= nullArchetypeIndex){
            uint32_t archetypeIndex = archetypes.size();
            if((archetypeIndex+1) >= nullArchetypeIndex)
                throw std::out_of_range("archtype index full");
            archetype->archetypeIndex = archetypeIndex;
            archetypes.push_back(std::move(archetype_ptr));
        }else{
            unique_ptr<Archetype>& ptr = archetypes.at(freeArchetypeIndex);
            /// TODO: may a invalid index check be good
            if(( (intptr_t)ptr.get_raw() & 0x1) != 1)
                throw std::runtime_error("getOrCreateArchetype(): unexpected free archetype");
            archetype->archetypeIndex = freeArchetypeIndex;
            freeArchetypeIndex = (intptr_t)ptr.get_raw() >> 1;
            ptr = std::move(archetype_ptr);
        }
        return archetype;
    }
}
void EntityComponentManager::destroyEmptyArchetype(const uint32_t archetypeIndex){
    // assuming archetypeIndex is less that nullArchetypeIndex
    Archetype * arch = this->archetypes[archetypeIndex].get();
    if(arch && arch->empty()) {
        //printf("Debug: destroy archetype bitmask %u\n",this->archetypes_id[archetype_index]);
        this->archetypes[archetypeIndex].reset((Archetype*)( (intptr_t)(freeArchetypeIndex << 1)&1 ) );
        freeArchetypeIndex = archetypeIndex;
    }
}
Archetype *EntityComponentManager::getArchetypeWithAddedComponents(Archetype *archetype,span<TypeIndex> componentTypeSet){
    span<TypeIndex> srcTypes = archetype->types;
    uint32_t dstTypesCount = srcTypes.size() + componentTypeSet.size();

    TypeIndex dstTypes[dstTypesCount];

    // zipper the two sorted arrays
    // because this is done in-place,
    // it must be done backwards so as not to disturb the existing contents.

    uint32_t unusedIndices = 0;
    {
        int32_t oldThings = srcTypes.size() - 1;
        int32_t newThings = componentTypeSet.size() - 1;
        uint32_t mixedThings = dstTypesCount;
        while (newThings >= 0) // oldThings[0] has value 0, newThings can't have anything lower than that
        {
            // there is no way oldThings be less than 0 if things goes currectly
            // type arrays being sorted and getTypeInfo<TypeIndex>().value.value() == 0
            TypeIndex oldThing = srcTypes[oldThings];
            TypeIndex newThing = componentTypeSet[newThings];
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
Archetype* EntityComponentManager::getArchetypeWithAddedComponent(Archetype* archetype, TypeIndex componentType,uint32_t *indexInTypeArray)
{
    span<TypeIndex> srcTypes = archetype->types;
    uint32_t dstTypesCount = srcTypes.size() + 1;

    TypeIndex newTypes[dstTypesCount];

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
Archetype* EntityComponentManager::getArchetypeWithRemovedComponents(Archetype *archetype,span<TypeIndex> typeSetToRemove){
    const uint32_t typesCount = archetype->types.size();
    TypeIndex newTypes[typesCount];

    uint32_t numRemovedTypes = 0;
    for (uint32_t t = 0; t < typesCount; ++t)
    {
        uint16_t existingTypeIndex = archetype->types[t].value;

        bool removed = false;
        for(const TypeIndex type : typeSetToRemove)
            if (existingTypeIndex == getTypeInfo(type).value.value)
            {
                numRemovedTypes++;
                removed = true;
                break;
            }

        if (!removed)
            newTypes[t - numRemovedTypes] = archetype->types[t];
    }

    if (numRemovedTypes == 0)
        return archetype;
    return getOrCreateArchetype({newTypes, typesCount - numRemovedTypes});
}
Archetype* EntityComponentManager::getArchetypeWithRemovedComponent (Archetype* archetype,TypeIndex addedComponentType,uint32_t *indexInOldTypeArray)
{
    const uint32_t typesCount = archetype->types.size();
    TypeIndex newTypes[typesCount];

    uint32_t removedTypes = 0;
    for (uint32_t t = 0; t < typesCount; ++t)
        if (archetype->types[t].value == addedComponentType.value)
        {
            if (indexInOldTypeArray != nullptr)
                *indexInOldTypeArray = t;
            ++removedTypes;
        }
        else
            newTypes[t - removedTypes] = archetype->types[t];

    return getOrCreateArchetype({newTypes, typesCount - removedTypes});
}




















Entity EntityComponentManager::createEntity(){
    return recycleEntity();
}
void EntityComponentManager::addComponents(Entity entity, span<TypeIndex> componentTypeSet)
{
    entity_t& srcValue = validate(entity);

    Archetype *srcArchetype,*newArchetype;
    uint32_t newIndex;

    if(srcValue.validArchtype()){
        //entity_value must contain valid value, any exception is our fault and flaw
        srcArchetype = this->archetypes.at(srcValue.archetype).get();
        newArchetype = this->getArchetypeWithAddedComponents(srcArchetype, componentTypeSet);
        if (srcArchetype == newArchetype)  // none were added
            return;
        //
        const uint32_t srcIndex = srcValue.index;

        newIndex = newArchetype->createEntity();

        Archetype::moveComponentValues(newArchetype,newIndex,srcArchetype,srcIndex);

        const uint32_t replacedIndex = srcArchetype->removeEntity(srcIndex);
        // if it index was filled with other entity
        if(replacedIndex!=srcIndex){
            const Entity replacedEntity = Archetype::replaceComponentValues(srcArchetype,srcIndex,replacedIndex);
            this->entity_value.at(replacedEntity.index()).index = srcIndex;
        }
    }else{
        newArchetype = this->getOrCreateArchetype(componentTypeSet);
        newIndex = newArchetype->createEntity();
        newArchetype->callComponentConstructor(newIndex,entity);
    }
    srcValue.index = newIndex;
    srcValue.archetype = newArchetype->archetypeIndex;
}
void EntityComponentManager::removeComponents(Entity entity, span<TypeIndex> componentTypeSet){
    entity_t& srcValue = validate(entity);

    Archetype *srcArchetype,*newArchetype;
    uint32_t newIndex;

    if(srcValue.validArchtype()){
        //entity_value must contain valid value, any exception is our fault and flaw
        srcArchetype = this->archetypes.at(srcValue.archetype).get();
        newArchetype = this->getArchetypeWithRemovedComponents(srcArchetype, componentTypeSet);
        if (srcArchetype == newArchetype)  // none were added
            return;
        const uint32_t srcIndex = srcValue.index;

        newIndex = newArchetype->createEntity();

        Archetype::moveComponentValues(newArchetype,newIndex,srcArchetype,srcIndex);

        const uint32_t replacedIndex = srcArchetype->removeEntity(srcIndex);
        // if it index was filled with other entity
        if(replacedIndex!=srcIndex){
            const Entity replacedEntity = Archetype::replaceComponentValues(srcArchetype,srcIndex,replacedIndex);
            this->entity_value.at(replacedEntity.index()).index = srcIndex;
        }
    }
}
Entity EntityComponentManager::createEntity(span<TypeIndex> types) {
    const Entity result = recycleEntity();
    if(!types.empty())
    {
        Archetype *arch = this->getOrCreateArchetype(types);
        entity_t &value = this->entity_value[result.index()];
        value.index = arch->createEntity();
        value.archetype = arch->archetypeIndex;
        arch->callComponentConstructor(value.index,result);
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

        Entity replacedEntity = Archetype::managedRemoveEntity(arch,value.index);
        // if it index was filled with other entity
        if(replacedEntity.valid())
                this->entity_value.at(replacedEntity.index()).index = value.index;
        value.archetype = nullArchetypeIndex;
    }
}

bool EntityComponentManager::hasComponents(Entity entity,span<TypeIndex> types) const {
    // code copyied from validate()
    if(this->entity_value.size() <= entity.index())
        return false;
    const entity_t value = this->entity_value[entity.index()];
    // version check to avoid double destroy
    if(value.version != entity.version())
        return false;

    if(types.size() < 1) return true;

    Archetype* arch = this->archetypes.at(value.archetype).get();

    return arch->hasComponents(types);
}

bool EntityComponentManager::hasComponent(Entity entity,TypeIndex type) const {
    // code copyied from validate()
    if(this->entity_value.size() <= entity.index())
        return false;
    const entity_t value = this->entity_value[entity.index()];
    // version check to avoid double destroy
    if(value.version != entity.version())
        return false;

    Archetype *arch = this->archetypes.at(value.archetype).get();
    return arch->hasComponent(type);
}




















/*

        template<typename Type>
        inline Type chunk_wrapper(const size_t chunk_index, const std::array<size_t,COMPOMEN_COUNT>& offsets, void*chunk){
            getTypeInfo<Type>().ko;
            return (Type*)(((char*)chunk) + offsets[type_id<Type>().index])[chunk_index];
        }

        template<typename Type>
        inline void init_wrapper(uint32_t index, const std::vector<std::pair<typeid_t,size_t>>& offsets, void*chunk, const Type& init_value){
            for(const auto& offset:offsets)
                if(offset.first == type_id<Type>().index)
                    new (((Type*)(((uint8_t*)chunk) + offset.second))+index) Type(init_value);
        }
}*/


void EntityComponentManager::recycleEntity(Entity entity){
    entity_t& value = this->entity_value[entity.index()];
    value.index = freeEntityIndex;
    value.archetype = nullArchetypeIndex;
    // increasing version prevent from double destroing objects
    value.version++;
    freeEntityIndex = entity.index();
}
Entity EntityComponentManager::recycleEntity(){
    Entity result;
    if(freeEntityIndex < Entity::maxEntityCount){
        entity_t& value = this->entity_value[freeEntityIndex];
        result = Entity{freeEntityIndex,value.version};
        // version and archetype mus be set on destroyEntity
        freeEntityIndex = value.index;
    }else{
        const uint32_t index = entity_value.size();
        if((index+1) >= Entity::maxEntityCount)
            throw std::out_of_range("createEntity(): entity count limit reached");
        entity_value.emplace_back();
        result = Entity{index,0};
    }
    return result;
}