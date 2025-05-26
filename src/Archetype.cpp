#include "ECS/Archetype.hpp"
using namespace ECS;


Archetype* Archetype::createArchetype(span<TypeIndex> types) {
    const size_t typesCount = types.size() + 1;
    if(unlikely(typesCount < 1 || typesCount > 0xFFF))
        throw std::invalid_argument("createArchetype(): archetype with unexpected component count");

    uint32_t additional = 0;
    uint16_t offsets[4];
    offsets[0] =              alignTo64(sizeof(Archetype),1);
    offsets[1] = offsets[0] + alignTo64(sizeof(TypeIndex),typesCount);
    offsets[2] = offsets[1] + alignTo64(sizeof(uint16_t),typesCount);
    offsets[3] = offsets[2] + alignTo64(sizeof(uint32_t),typesCount);
    additional = offsets[3] + alignTo64(sizeof(uint16_t),typesCount);


    Archetype *arch = (Archetype*) allocator().allocate(sizeof(Archetype) + additional);

    new (arch) Archetype();

    arch->types = span<TypeIndex>{(TypeIndex*)((uint8_t*)(arch) + offsets[0]),typesCount};
    arch->realIndecies = span<uint16_t>{(uint16_t*)((uint8_t*)(arch) + offsets[1]),typesCount};
    arch->offsets = span<uint32_t>{(uint32_t*)((uint8_t*)(arch) + offsets[2]),typesCount};
    arch->sizeOfs = span<uint16_t>{(uint16_t*)((uint8_t*)(arch) + offsets[3]),typesCount};

    // TODO: this value must be always 0
    arch->types[0] = getTypeInfo<Entity>().value;
    if(unlikely(arch->types[0].value != 0))
        throw std::runtime_error("createArchetype(): getTypeInfo<Entity>().value.realIndex() must be always 0!");
    arch->realIndecies[0] = 0;


    for (uint32_t i = 0;i < types.size(); i++) {
        arch->types[i+1]=types[i];
        arch->realIndecies[i+1] = types[i].realIndex();
    }

    arch->chunks.initialize(typesCount);


    {
        uint16_t i = typesCount;
        do arch->firstTagIndex = i;
        while (i!=0 && getTypeInfo(arch->types[--i]).size == 0);
        i++;
    }
    for (uint32_t i = 0; i < typesCount; ++i)
    {
        if (i < arch->firstTagIndex){
            const auto& cType = getTypeInfo(arch->types[i]);
            arch->sizeOfs[i] = cType.size;
        }else
            arch->sizeOfs[i] = 0;
    }
    for (TypeIndex type: arch->types)
    {
        if(type.isPrefab())
            arch->flags |= TypeIndex::prefab;
    }

    arch->chunkCapacity = std::min( 
        calculateChunkCapacity(Chunk::bufferSize,arch->sizeOfs) , 
        Chunk::maximumEntitiesPerChunk
    );

    if(unlikely(arch->chunkCapacity < 2))
        throw std::length_error("createArchetype(): LARGE COMPONENTS! X(");

    uint32_t usedBytes = Chunk::memoryOffset;
    for (uint32_t typeIndex = 0; typeIndex < typesCount; typeIndex++)
    {
        uint32_t sizeOf = arch->sizeOfs[typeIndex];

        // align usedBytes upwards (eating into alignExtraSpace) so that
        // this component actually starts at its required alignment.
        // Assumption is that the start of the entire data segment is at the
        // maximum possible alignment.
        arch->offsets[typeIndex] = usedBytes;
        usedBytes += alignTo64(sizeOf, arch->chunkCapacity);
    }

    return arch;
}

uint32_t Archetype::createEntity() {
    uint32_t res = 0;
    if(unlikely(lastChunkEntityCount == 0 && !this->chunksData.empty()))
        throw std::runtime_error("createEntity(): intrnal error!");
    if(unlikely(this->chunksData.empty() || lastChunkEntityCount >= this->chunkCapacity)){
        lastChunkEntityCount=1;
        this->chunksData.emplace_back().memory = allocator().allocate(Chunk::memorySize);
        chunks.add(0);
    }else
        ++lastChunkEntityCount;

    res = this->count() - 1;

    if(unlikely(res >= nullEntityIndex))
        throw std::out_of_range("entity count limit reached");
    return res;
}

uint32_t Archetype::removeEntity(uint32_t entityIndex){
    if(unlikely(this->chunksData.size() < 1))
        throw std::runtime_error("removeEntity(): empty archetype");
    if(unlikely(this->lastChunkEntityCount < 1))
        throw std::runtime_error("removeEntity(): unexpected internal error");

    uint32_t lastEntityIndex = count() - 1;

    if(unlikely(lastEntityIndex < entityIndex))
            throw std::invalid_argument("removeEntity(): entity does not exists");

    this->lastChunkEntityCount--;

    // 0 means last element on the chunk is either moved to
    // the deleted entity position or is the deleted entity itself
    // so chunk must be cleared
    if(this->lastChunkEntityCount == 0){
        this->chunksData.pop_back();
        this->chunks.popBack();
        if(this->chunksData.size() != 0)
            this->lastChunkEntityCount = this->chunkCapacity;
    }
    return lastEntityIndex;
}

void* Archetype::getComponent(uint16_t componentIndex,uint32_t entityIndex){
    uint32_t inChunkIndex = getInChunkIndex(entityIndex);
    uint32_t chunkIndex = getChunkIndex(entityIndex);
    uint8_t *chunkMemory = (uint8_t *)chunksData.at(chunkIndex).memory;
    if(unlikely((chunkIndex+1) >= chunksData.size() && inChunkIndex >= lastChunkEntityCount))
            throw std::out_of_range("getComponent(): invalid entityIndex");
    return chunkMemory + offsets.at(componentIndex) + (sizeOfs.at(componentIndex) * inChunkIndex);
}

int32_t Archetype::getIndex(TypeIndex t){
    uint16_t rIndex = t.realIndex();
    for (size_t i = 0; i < realIndecies.size(); i++){
        if(realIndecies[i] == rIndex)
            return i;
        else if(realIndecies[i] > rIndex)
            return -1;
    }
    return -1;
}











bool Archetype::hasComponent(TypeIndex type) {
    span<TypeIndex> archetypeTypes = this->types;
    if(archetypeTypes.size() < 1) return false;

    uint16_t archetypeTypesIndex = 0;

    while(archetypeTypesIndex < archetypeTypes.size())
    {
        auto archetypeType = archetypeTypes[archetypeTypesIndex];
        // TODO: compaire number of remainding types
        if(archetypeType.value < type.value){
            archetypeTypesIndex++;
        }else if(archetypeType.value > type.value){
            return false;
        }else{
            if(archetypeType.flag != type.flag)
                return false;
            return true;
        }
    }
    return false;
}
bool Archetype::hasComponents(span<TypeIndex> rtypes)
{
    span<TypeIndex> archetypeTypes = this->types;
    if(archetypeTypes.size() < rtypes.size()) return false;

    uint16_t archetypeTypesIndex = 0;
    uint16_t typesIndex = 0;

    while(typesIndex < rtypes.size() &&
        archetypeTypesIndex < archetypeTypes.size())
    {
        auto archetypeType = archetypeTypes[archetypeTypesIndex],
            type = rtypes[typesIndex];
        // TODO: compaire number of remainding types
        if(archetypeType.value == type.value){
            if(archetypeType.flag != type.flag)
                return false;
            archetypeTypesIndex++;
            typesIndex++;
        }else  if(archetypeType.value > type.value){
            return false;
        }else{
            archetypeTypesIndex++;
        }
    }
    if(typesIndex == rtypes.size())
        return true;
    return false;
}
Entity Archetype::managedRemoveEntity(Archetype *archetype, uint32_t entityIndex){
    if(unlikely(archetype->chunksData.size() < 1))
        throw std::runtime_error("removeEntity(): empty archetype");
    if(unlikely(archetype->lastChunkEntityCount < 1))
        throw std::runtime_error("removeEntity(): unexpected internal error");

    uint32_t lastEntityIndex = archetype->count() - 1;

    if(unlikely(lastEntityIndex < entityIndex))
        throw std::invalid_argument("removeEntity(): entity does not exists");

    archetype->lastChunkEntityCount--;

    Entity ret = Entity::null;
    if(unlikely(lastEntityIndex !=  entityIndex))
        ret = moveComponentValues(archetype,entityIndex,lastEntityIndex);
    else
        archetype->callComponentDestructor(entityIndex);

    // 0 means last element on the chunk is either moved to
    // the deleted entity position or is the deleted entity itself
    // so chunk must be cleared
    if(archetype->lastChunkEntityCount == 0){
        archetype->chunksData.pop_back();
        if(archetype->chunksData.size() != 0)
            archetype->lastChunkEntityCount = archetype->chunkCapacity;
        archetype->chunks.popBack();
    }
    return ret;
}
Entity Archetype::moveComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex)
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
Entity Archetype::replaceComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex)
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
Entity Archetype::moveComponentValues(Archetype *dstArchetype, uint32_t dstIndex,Archetype *srcArchetype, uint32_t srcIndex){
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
void Archetype::callComponentDestructor(uint32_t entityIndex){

    uint16_t const * const typeIndex = this->realIndecies.data();
    uint32_t const * const typeOffset = this->offsets.data();
    uint16_t const * const typeSize = this->sizeOfs.data();
    uint8_t   * const chunk = (uint8_t *)this->chunksData[this->getChunkIndex(entityIndex)].memory;
    const uint32_t srcInChunkIndex = this->getInChunkIndex(entityIndex);
    uint16_t count = this->nonZeroSizedTypesCount();

    for(uint16_t cIndex = 0;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.destructor)
            info.destructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
void Archetype::callComponentConstructor(uint32_t entityIndex, Entity e)
{
    uint16_t const * const typeIndex = this->realIndecies.data();
    uint32_t const * const typeOffset = this->offsets.data();
    uint16_t const * const typeSize = this->sizeOfs.data();
    uint8_t   * const chunk = (uint8_t *)this->chunksData[this->getChunkIndex(entityIndex)].memory;
    const uint32_t srcInChunkIndex = this->getInChunkIndex(entityIndex);
    uint16_t count = this->nonZeroSizedTypesCount();

    ((Entity*)(chunk+typeOffset[0]))[srcInChunkIndex] = e;

    for(uint16_t cIndex = 1;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.constructor)
            info.constructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
