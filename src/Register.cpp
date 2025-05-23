#include "ECS/Register.hpp"
using namespace DOTS;




Archetype* Register::getOrCreateArchetype(span<TypeIndex> types){
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
void Register::destroyEmptyArchetype(const uint32_t archetypeIndex){
    // assuming archetypeIndex is less that nullArchetypeIndex
    Archetype * arch = this->archetypes[archetypeIndex].get();
    if(arch && arch->empty()) {
        //printf("Debug: destroy archetype bitmask %u\n",this->archetypes_id[archetype_index]);
        this->archetypes[archetypeIndex].reset((Archetype*)( (intptr_t)(freeArchetypeIndex << 1)&1 ) );
        freeArchetypeIndex = archetypeIndex;
    }
}
Archetype *Register::getArchetypeWithAddedComponents(Archetype *archetype,span<TypeIndex> componentTypeSet){
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
Archetype* Register::getArchetypeWithAddedComponent(Archetype* archetype, TypeIndex componentType)
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

    if (t != srcTypes.size() &&
        srcTypes[t].value == componentType.value &&
        srcTypes[t].flag == componentType.flag) {
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





















Entity Register::createEntity(){
    return recycleEntity();
}
void Register::addComponent(Entity entity, span<TypeIndex> componentTypeSet)
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

        moveComponentValues(newArchetype,newIndex,srcArchetype,srcIndex);

        const uint32_t replacedIndex = srcArchetype->removeEntity(srcIndex);
        // if it index was filled with other entity
        if(replacedIndex!=srcIndex){
            const Entity replacedEntity = replaceComponentValues(srcArchetype,srcIndex,replacedIndex);
            this->entity_value.at(replacedEntity.index()).index = srcIndex;
        }
    }else{
        newArchetype = this->getOrCreateArchetype(componentTypeSet);
        newIndex = newArchetype->createEntity();
        callComponentConstructor(newArchetype,newIndex,entity);
    }
    srcValue.index = newIndex;
    srcValue.archetype = newArchetype->archetypeIndex;
}
Entity Register::createEntity(span<TypeIndex> types) {
    const Entity result = recycleEntity();
    if(!types.empty())
    {
        Archetype *arch = this->getOrCreateArchetype(types);
        entity_t &value = this->entity_value[result.index()];
        value.index = arch->createEntity();
        value.archetype = arch->archetypeIndex;
        callComponentConstructor(arch,value.index,result);
    }
    return result;
}
void Register::destroyEntity(Entity entity) {
    validate(entity);

    removeComponents(entity);
    recycleEntity(entity);
}
void Register::removeComponents(Entity entity)
{
    entity_t& value = validate(entity);

    // value.validArchtype() isnt enough
    if(value.validArchtype()){
        Archetype *arch = this->archetypes.at(value.archetype).get();
        if(!arch)
            throw std::runtime_error("destroyEntity(): entity points to invalid archetype");

        const uint32_t replacedIndex = arch->removeEntity(value.index);
        // if it index was filled with other entity
        if(replacedIndex!=value.index){
            const Entity replacedEntity = moveComponentValues(arch,value.index,replacedIndex);
            if(replacedEntity.valid())
                this->entity_value.at(replacedEntity.index()).index = value.index;
        }else
            callComponentDestructor(arch,replacedIndex);
        value.archetype = nullArchetypeIndex;
    }
}

bool Register::hasComponents(Entity entity,span<TypeIndex> types) const {
    // code copyied from validate()
    if(this->entity_value.size() <= entity.index())
        return false;
    const entity_t value = this->entity_value[entity.index()];
    // version check to avoid double destroy
    if(value.version != entity.version())
        return false;

    if(types.size() < 1) return true;

    span<uint16_t> archetypeTypes = this->archetypes.at(value.archetype).get()->realIndecies;
    if(archetypeTypes.size() < types.size()) return false;

    uint16_t archetypeTypesIndex = 0;
    uint16_t typesIndex = 0;

    while(typesIndex < types.size() &&
        archetypeTypesIndex < archetypeTypes.size())
    {
        // TODO: compaire number of remainding types
        if(archetypeTypes[archetypeTypesIndex] == types[typesIndex].value){
            archetypeTypesIndex++;
            typesIndex++;
        }else  if(archetypeTypes[archetypeTypesIndex] > types[typesIndex].value){
            return false;
        }else/*if(archetypeTypes[archetypeTypesIndex] <  types[typesIndex].value)*/{
            archetypeTypesIndex++;
        }
    }
    if(typesIndex == types.size())
        return true;
    return false;
}

bool Register::hasComponent(Entity entity,TypeIndex type) const {
    // code copyied from validate()
    if(this->entity_value.size() <= entity.index())
        return false;
    const entity_t value = this->entity_value[entity.index()];
    // version check to avoid double destroy
    if(value.version != entity.version())
        return false;

    span<uint16_t> archetypeTypes = this->archetypes.at(value.archetype).get()->realIndecies;
    if(archetypeTypes.size() < 1) return false;

    uint16_t archetypeTypesIndex = 0;

    while(archetypeTypesIndex < archetypeTypes.size())
    {
        // TODO: compaire number of remainding types
        if(archetypeTypes[archetypeTypesIndex] < type.value){
            archetypeTypesIndex++;
        }else  if(archetypeTypes[archetypeTypesIndex] > type.value){
            return false;
        }else/*if(archetypeTypes[archetypeTypesIndex] ==  types[typesIndex].value)*/{
            return true;
        }
    }
    return false;
}




















/*


        /// @brief add list of components, components are initialized with default constructor
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        template<typename ... Args>
        void addComponent(Entity entity){
            entity_t& srcValue = validate(entity);

            Archetype *srcArchetype,*newArchetype;
            uint32_t newIndex;

            if(srcValue.validArchtype()){
                std::vector<TypeIndex> componentTypeSet = componentTypes<Args...>();
                //entity_value must contain valid value, any exception is our fault and flaw
                srcArchetype = this->archetypes.at(srcValue.archetype).get();
                newArchetype = this->getArchetypeWithAddedComponents(srcArchetype, componentTypeSet);
                if (srcArchetype == newArchetype)  // none were added
                    return;

                auto v = moveComponentValuesManaged(newArchetype,srcArchetype,srcValue.index);
                if(v.second.valid())
                    this->entity_value.at(v.second.index()).index = srcValue.index;
                srcValue.index = v.first;
                srcValue.archetype = newArchetype->archetypeIndex;

            }else{
                std::vector<TypeIndex> componentTypeSet = componentTypesForArchetype<Args...>();
                newArchetype = this->getOrCreateArchetype(componentTypeSet);
                srcValue.index = newEntity(newArchetype,entity);
                srcValue.archetype = newArchetype->archetypeIndex;
            }
        }


uint32_t Register::newEntity(Archetype *archetype, Entity e){
    uint32_t newIndex = archetype->createEntity();
    callComponentConstructor(archetype,newIndex,e);
    return newIndex;
}
std::pair<uint32_t,Entity> Register::moveComponentValuesManaged(Archetype *dstArchetype,Archetype *srcArchetype, uint32_t srcIndex)
{
    std::pair<uint32_t,Entity> ret;
    // srcIndex
    ret.first = dstArchetype->createEntity();
    moveComponentValues(dstArchetype,ret.first,srcArchetype,srcIndex);

    const uint32_t replacedIndex = srcArchetype->removeEntity(srcIndex);
    // if it index was filled with other entity
    if(replacedIndex!=srcIndex)
        // replacedEntity
        ret.second = replaceComponentValues(srcArchetype,srcIndex,replacedIndex);
    return ret;
}*/


void Register::recycleEntity(Entity entity){
    entity_t& value = this->entity_value[entity.index()];
    value.index = freeEntityIndex;
    value.archetype = nullArchetypeIndex;
    // increasing version prevent from double destroing objects
    value.version++;
    freeEntityIndex = entity.index();
}
Entity Register::recycleEntity(){
    Entity result;
    if(freeEntityIndex < Entity::maxEntityCount){
        entity_t& value = this->entity_value[freeEntityIndex];
        result = Entity{freeEntityIndex,value.version};
        // version and archetype mus be set on destroyEntity
        freeEntityIndex = value.index;
    }else{
        const uint32_t index = entity_value.size();
        if((index+1) >= Entity::maxEntityCount)
            throw std::out_of_range("createEntity(): register limit reached");
        entity_value.emplace_back();
        result = Entity{index,0};
    }
    return result;
}
Entity Register::moveComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex)
{
    Entity ret;
    const uint32_t dstChunkIndex = archetype->getChunkIndex(dstIndex);
    const uint32_t dstInChunkIndex = archetype->getInChunkIndex(dstIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const dstChunkMemory = (uint8_t*) archetype->chunksData.at(dstChunkIndex).memory;

    const uint32_t srcChunkIndex = archetype->getChunkIndex(srcIndex);
    const uint32_t srcInChunkIndex = archetype->getInChunkIndex(srcIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const srcChunkMemory = (uint8_t*) archetype->chunksData.at(srcChunkIndex).memory;

    for(uint32_t i=0;i < archetype->nonZeroSizedTypesCount();i++) {
        const auto& info = getTypeInfo(archetype->types[i]);
        if(info.destructor)
        {
            uint8_t * const ptr1 =
                dstChunkMemory +
                archetype->offsets[i] +
                (archetype->sizeOfs[i] * dstInChunkIndex);
            info.destructor(ptr1);
        }
    }

    ret = ((Entity*)(srcChunkMemory+archetype->offsets[0]))[srcInChunkIndex];

    for(uint32_t i=0;i < archetype->nonZeroSizedTypesCount();i++) {
        uint8_t const * const src =
                srcChunkMemory +
                archetype->offsets[i] +
                (archetype->sizeOfs[i] * srcInChunkIndex);
        uint8_t * const dst =
                dstChunkMemory +
                archetype->offsets[i] +
                (archetype->sizeOfs[i] * dstInChunkIndex);
        memcpy(dst,src,archetype->sizeOfs[i]);
    }

    return ret;
}
Entity Register::replaceComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex)
{
    Entity ret;
    const uint32_t dstChunkIndex = archetype->getChunkIndex(dstIndex);
    const uint32_t dstInChunkIndex = archetype->getInChunkIndex(dstIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const dstChunkMemory = (uint8_t*) archetype->chunksData.at(dstChunkIndex).memory;

    const uint32_t srcChunkIndex = archetype->getChunkIndex(srcIndex);
    const uint32_t srcInChunkIndex = archetype->getInChunkIndex(srcIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const srcChunkMemory = (uint8_t*) archetype->chunksData.at(srcChunkIndex).memory;

    ret = ((Entity*)(srcChunkMemory+archetype->offsets[0]))[srcInChunkIndex];

    for(uint32_t i=0;i < archetype->nonZeroSizedTypesCount();i++) {
        uint8_t const * const src =
                srcChunkMemory +
                archetype->offsets[i] +
                (archetype->sizeOfs[i] * srcInChunkIndex);
        uint8_t * const dst =
                dstChunkMemory +
                archetype->offsets[i] +
                (archetype->sizeOfs[i] * dstInChunkIndex);
        memcpy(dst,src,archetype->sizeOfs[i]);
    }

    return ret;
}
Entity Register::moveComponentValues(Archetype *dstArchetype, uint32_t dstIndex,Archetype *srcArchetype, uint32_t srcIndex){
    Entity ret;
    uint16_t const * const srcTypeIndex = srcArchetype->realIndecies.data();
    uint32_t const * const srcTypeOffset = srcArchetype->offsets.data();
    uint16_t const * const srcTypeSize = srcArchetype->sizeOfs.data();
    uint8_t   * const srcChunk = (uint8_t *)srcArchetype->chunksData[srcArchetype->getChunkIndex(srcIndex)].memory;
    const uint32_t srcInChunkIndex = srcArchetype->getInChunkIndex(srcIndex);

    uint16_t const *const dstTypeIndex = dstArchetype->realIndecies.data();
    uint32_t const *const dstTypeOffset = dstArchetype->offsets.data();
    uint16_t const *const dstTypeSize = dstArchetype->sizeOfs.data();
    uint8_t * const dstChunk = (uint8_t *)dstArchetype->chunksData[dstArchetype->getChunkIndex(dstIndex)].memory;
    const uint32_t dstInChunkIndex = dstArchetype->getInChunkIndex(dstIndex);


    uint16_t srcCount = srcArchetype->nonZeroSizedTypesCount();
    uint16_t dstCount = dstArchetype->nonZeroSizedTypesCount();

    uint16_t srcI = 0,
    dstI = 0;
    {
        // for madvanced cache friendly-ness

        uint16_t toBeDeletedCount=0;
        uint16_t toBeInitCount=0;
        uint16_t toBeDeletedArray[srcCount];
        uint16_t toBeInitArray[dstCount];

        ret = ((Entity*)(srcChunk+srcArchetype->offsets[0]))[srcInChunkIndex];
        // for better ilutration imagine this:
        // dstTypeIndex: [0 3 4 5 7]
        // srcTypeIndex: [0 1 4 5]
        // -> copy(dst[0],src[0]), destroy(src[1]), init(dst[1]), copy(dest[2],src[2]), ...

        uint16_t shareCount = std::min(srcCount,dstCount);
        while (shareCount)
        {
            if(srcTypeIndex[srcI] == dstTypeIndex[dstI]){
                memcpy(
                    dstChunk + dstTypeOffset[dstI] + dstTypeSize[dstI] * dstInChunkIndex,
                    srcChunk + srcTypeOffset[srcI] + dstTypeSize[srcI] * srcInChunkIndex,
                    dstTypeSize[dstI]);
                ++srcI;
                ++dstI;
            }else if(srcTypeIndex[srcI] < dstTypeIndex[dstI]){
                // destination is missing this component
                // so it can be destroyed
                toBeDeletedArray[toBeDeletedCount++] = srcI++;
            }else{
                // src is missing a component
                // so init it for dst
                toBeInitArray[toBeInitCount++] = dstI++;
            }
        }

        while(toBeDeletedCount){
            --toBeDeletedCount;
            uint16_t toBeDeleted = toBeDeletedArray[toBeDeletedCount];
            auto& info = getTypeInfo(srcTypeIndex[toBeDeleted]);
            if(info.destructor)
                info.destructor(srcChunk + srcTypeOffset[toBeDeleted] + srcTypeSize[toBeDeleted] * srcInChunkIndex);
        }
        for(;srcI < srcCount;){
            auto& info = getTypeInfo(srcTypeIndex[srcI]);
            if(info.destructor)
                info.destructor(srcChunk + srcTypeOffset[srcI] + srcTypeSize[srcI] * srcInChunkIndex);
            ++srcI;
        }

        while(toBeInitCount){
            --toBeInitCount;
            uint16_t toBeInit = toBeInitArray[toBeInitCount];
            auto& info = getTypeInfo(dstTypeIndex[toBeInit]);
            if(info.constructor)
                info.constructor(dstChunk + dstTypeOffset[toBeInit] + dstTypeSize[toBeInit] * dstInChunkIndex);
        }
        for(;dstI < dstCount;){
            auto& info = getTypeInfo(dstTypeIndex[dstI]);
            if(info.constructor)
                info.constructor(dstChunk + dstTypeOffset[dstI] + dstTypeSize[dstI] * dstInChunkIndex);
            ++dstI;
        }
    }
    return ret;
}
void Register::callComponentDestructor(Archetype *arch, uint32_t entityIndex){

    uint16_t const * const typeIndex = arch->realIndecies.data();
    uint32_t const * const typeOffset = arch->offsets.data();
    uint16_t const * const typeSize = arch->sizeOfs.data();
    uint8_t   * const chunk = (uint8_t *)arch->chunksData[arch->getChunkIndex(entityIndex)].memory;
    const uint32_t srcInChunkIndex = arch->getInChunkIndex(entityIndex);
    uint16_t count = arch->nonZeroSizedTypesCount();

    for(uint16_t cIndex = 0;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.destructor)
            info.destructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
void Register::callComponentConstructor(Archetype *arch, uint32_t entityIndex, Entity e)
{
    uint16_t const * const typeIndex = arch->realIndecies.data();
    uint32_t const * const typeOffset = arch->offsets.data();
    uint16_t const * const typeSize = arch->sizeOfs.data();
    uint8_t   * const chunk = (uint8_t *)arch->chunksData[arch->getChunkIndex(entityIndex)].memory;
    const uint32_t srcInChunkIndex = arch->getInChunkIndex(entityIndex);
    uint16_t count = arch->nonZeroSizedTypesCount();

    ((Entity*)(chunk+typeOffset[0]))[srcInChunkIndex] = e;

    for(uint16_t cIndex = 1;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.constructor)
            info.constructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
