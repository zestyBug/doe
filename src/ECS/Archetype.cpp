#include "ECS/Archetype.hpp"
#include "cutil/HashHelper.hpp"
using namespace ECS;


ArchetypeHolder Archetype::createArchetype(const_span<TypeID> types) {
    if(unlikely(types.size() < 1 || types.size() > MaxTypePerArchetype))
        throw std::invalid_argument("createArchetype(): archetype with unexpected component count");

    uint32_t additional = 0;
    uint16_t offsets[4];
    offsets[0] =              (uint16_t)alignTo64(sizeof(Archetype),1);
    offsets[1] = offsets[0] + (uint16_t)alignTo64(sizeof(TypeID),types.size());
    offsets[2] = offsets[1] + (uint16_t)alignTo64(sizeof(uint16_t),types.size());
    offsets[3] = offsets[2] + (uint16_t)alignTo64(sizeof(uint32_t),types.size());
    additional = offsets[3] + (uint16_t)alignTo64(sizeof(uint16_t),types.size());

    ArchetypeHolder archHolder{(Archetype*) allocator().allocate(sizeof(Archetype) + additional)};
    Archetype *arch = archHolder.get();

    new (arch) Archetype();

    TypeID *arch_types;
    uint16_t *arch_realIndecies;
    uint32_t *arch_offsets;
    uint16_t *arch_sizeOfs;

    arch_types        = (TypeID*)((uint8_t*)(arch) + offsets[0]);
    arch_realIndecies = (uint16_t*)((uint8_t*)(arch) + offsets[1]);
    arch_offsets      = (uint32_t*)((uint8_t*)(arch) + offsets[2]);
    arch_sizeOfs      = (uint16_t*)((uint8_t*)(arch) + offsets[3]);
    
    arch->types        = const_span<TypeID>  {arch_types,types.size()};
    arch->realIndecies = const_span<uint16_t>{arch_realIndecies,types.size()};
    arch->offsets      = const_span<uint32_t>{arch_offsets,types.size()};
    arch->sizeOfs      = const_span<uint16_t>{arch_sizeOfs,types.size()};

#ifdef DEBUG
    if(unlikely(types[0].value != 0))
        throw std::runtime_error("createArchetype(): getTypeInfo<Entity>().value.realIndex() must be always 0!");
#endif


    for (uint32_t i = 0;i < types.size(); i++) {
        arch_types[i] = types[i];
        arch_realIndecies[i] = types[i].realIndex();
    }

    arch->chunksData.reserve(16);
    arch->chunksVersion.initialize(types.size());


    {
        uint16_t i = (uint16_t) types.size();
        do arch->firstTagIndex = i;
        while (i!=0 && getTypeInfo(arch->types[--i]).size == 0);
        i++;
    }
    for (uint32_t i = 0; i < types.size(); ++i)
    {
        if (i < arch->firstTagIndex){
            const auto& cType = getTypeInfo(arch->types[i]);
            arch_sizeOfs[i] = (uint16_t) cType.size;
        }else
            arch_sizeOfs[i] = 0;
    }
    for (TypeID type: arch->types)
    {
        if(type.isPrefab())
            arch->flags |= TypeID::prefab;
    }

    arch->chunkCapacity = std::min( 
        calculateChunkCapacity(Chunk::bufferSize,arch->sizeOfs) , 
        Chunk::maximumEntitiesPerChunk
    );

    if(unlikely(arch->chunkCapacity < 2))
        throw std::length_error("createArchetype(): LARGE COMPONENTS! X(");

    uint32_t usedBytes = Chunk::memoryOffset;
    for (uint32_t typeIndex = 0; typeIndex < types.size(); typeIndex++)
    {
        uint32_t sizeOf = arch->sizeOfs[typeIndex];
        arch_offsets[typeIndex] = usedBytes;
        usedBytes += alignTo64(sizeOf, arch->chunkCapacity);
    }

    return std::move(archHolder);
}

uint32_t Archetype::createEntity(version_t globalVersion) {
    uint32_t res = 0;
    res = this->count();
#ifdef DEBUG
    if(unlikely(lastChunkEntityCount == 0 && !this->chunksData.empty()))
        throw std::runtime_error("createEntity(): internal error!");
#endif
    if(unlikely(this->chunksData.empty() || lastChunkEntityCount >= this->chunkCapacity)){
        lastChunkEntityCount=1;
        this->chunksData.emplace_back().memory = allocator().allocate(Chunk::memorySize);
        if(this->chunksData.size() > Archetype::MaxChunkIndex)
            throw std::runtime_error("createEntity(): reached max chunk count");
        chunksVersion.add(globalVersion);
    }else{
        ++lastChunkEntityCount;
        chunksVersion.setAllChangeVersion((uint32_t)chunksData.size()-1,globalVersion);
    }

    if(unlikely(res > MaxEntityIndex))
        throw std::out_of_range("entity count limit reached");
    return res;
}

uint32_t Archetype::removeEntity(uint32_t entityIndex){
    if(unlikely(this->chunksData.size() < 1))
        throw std::runtime_error("removeEntity(): empty archetype");
#ifdef DEBUG
    if(unlikely(this->lastChunkEntityCount < 1))
        throw std::runtime_error("removeEntity(): unexpected internal error");
#endif

    uint32_t lastEntityIndex = count() - 1;

    if(unlikely(lastEntityIndex < entityIndex))
            throw std::invalid_argument("removeEntity(): entity does not exists");

    this->lastChunkEntityCount--;

    // 0 means last element on the chunk is either moved to
    // the deleted entity position or is the deleted entity itself
    // so chunk must be cleared
    if(this->lastChunkEntityCount == 0){
        this->chunksData.pop_back();
        this->chunksVersion.popBack();
        if(this->chunksData.size() != 0)
            this->lastChunkEntityCount = this->chunkCapacity;
    }
    return lastEntityIndex;
}

void* Archetype::getComponent(uint16_t componentIndex,uint32_t entityIndex, version_t newVersion){
    const uint32_t inChunkIndex = getInChunkIndex(entityIndex);
    const uint32_t chunkIndex = getChunkIndex(entityIndex);
    chunksVersion.getChangeVersion(componentIndex,chunkIndex) = newVersion;
    uint8_t *chunkMemory = (uint8_t *)chunksData.at(chunkIndex).memory;
    if(unlikely((chunkIndex+1) >= chunksData.size() && inChunkIndex >= lastChunkEntityCount))
            throw std::out_of_range("getComponent(): invalid entityIndex");
    return chunkMemory + offsets.at(componentIndex) + (sizeOfs.at(componentIndex) * inChunkIndex);
}
const void* Archetype::getComponent(uint16_t componentIndex,uint32_t entityIndex) const {
    const uint32_t inChunkIndex = getInChunkIndex(entityIndex);
    const uint32_t chunkIndex = getChunkIndex(entityIndex);
    const uint8_t *chunkMemory = (const uint8_t *)chunksData.at(chunkIndex).memory;
    if(unlikely((chunkIndex+1) >= chunksData.size() && inChunkIndex >= lastChunkEntityCount))
            throw std::out_of_range("getComponent(): invalid entityIndex");
    return chunkMemory + offsets.at(componentIndex) + (sizeOfs.at(componentIndex) * inChunkIndex);
}

int32_t Archetype::getIndex(TypeID t) const noexcept {
    uint16_t rIndex = t.realIndex();
    for (uint32_t i = 0; i < realIndecies.size(); i++){
        if(realIndecies[i] == rIndex)
            return i;
        else if(realIndecies[i] > rIndex)
            return -1;
    }
    return -1;
}
bool Archetype::getIndecies(const_span<TypeID> rtypes,uint16_t* out) const noexcept {
    const_span<TypeID> archetypeTypes = this->types;

    uint16_t archetypeTypesIndex = 0;
    uint16_t typesIndex = 0;

    while(typesIndex < rtypes.size() && archetypeTypesIndex < archetypeTypes.size())
    {
        TypeID archetypeType = archetypeTypes[archetypeTypesIndex],
               type          = rtypes[typesIndex];
        // TODO: compaire number of remainding types
        if(archetypeType.exactSame(type)){
            out[typesIndex++] = archetypeTypesIndex;
            // may contain duplicated items in the "types" parameter! archetypeTypesIndex++;
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
uint32_t Archetype::getHash() const noexcept {
    uint32_t result = HashHelper::FNV1A32(this->types);
    if (result == 0xFFFFFFFF || result == 0)
        result = 1;
    return result;
}










bool Archetype::hasComponent(TypeID type) const {
    const_span<TypeID> archetypeTypes = this->types;
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
bool Archetype::hasComponentsSlow(const_span<TypeID> rtypes) const {
    const_span<TypeID> archetypeTypes = this->types;
    if(archetypeTypes.size() < rtypes.size()) return false;

    uint16_t archetypeTypesIndex = 0;
    uint16_t typesIndex = 0;
    
    for(;typesIndex < rtypes.size();typesIndex++){
        for(;archetypeTypesIndex < archetypeTypes.size();archetypeTypesIndex++)
            if(archetypeTypes[archetypeTypesIndex].value == rtypes[typesIndex].value)
                goto endLoop;
        return false;
        endLoop:;
    }
    return false;
}
bool Archetype::hasComponents(const_span<TypeID> rtypes) const {
    const_span<TypeID> archetypeTypes = this->types;
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
#ifdef DEBUG
    if(unlikely(archetype->lastChunkEntityCount < 1))
        throw std::runtime_error("removeEntity(): unexpected internal error");
#endif

    uint32_t lastEntityIndex = archetype->count() - 1;

    if(unlikely(lastEntityIndex < entityIndex))
        throw std::invalid_argument("removeEntity(): entity does not exists");

    archetype->lastChunkEntityCount--;

    Entity ret = Entity::null;
    if(likely(lastEntityIndex !=  entityIndex))
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
        archetype->chunksVersion.popBack();
    }
    return ret;
}
Entity Archetype::moveComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex) {
#ifdef DEBUG
    if(dstIndex == srcIndex)
        throw std::invalid_argument("moveComponentValues(): unexpected same entity");
#endif
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

    if(srcChunkMemory == dstChunkMemory)
        archetype->inArchetypeCopy(srcChunkMemory, srcInChunkIndex, dstInChunkIndex);
    else
        archetype->inArchetypeCopy(srcChunkMemory, dstChunkMemory, srcInChunkIndex, dstInChunkIndex);
    return ret;
}
Entity Archetype::copyComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex) {
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

    if(srcChunkMemory == dstChunkMemory)
        archetype->inArchetypeCopy(srcChunkMemory, srcInChunkIndex, dstInChunkIndex);
    else
        archetype->inArchetypeCopy(srcChunkMemory, dstChunkMemory, srcInChunkIndex, dstInChunkIndex);

    return ret;
}
Entity Archetype::moveComponentValues(Archetype *__restrict__ dstArchetype, Archetype *__restrict__ srcArchetype, uint32_t dstIndex, uint32_t srcIndex){
#ifdef DEBUG
    if(dstArchetype == srcArchetype)
        throw std::invalid_argument("moveComponentValues(): unexpected same archetype");
#endif
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
        // for advanced cache friendly-ness

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
        while (shareCount--)
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
void Archetype::inArchetypeCopy(
    void *__restrict__ srcChunk, 
    void *__restrict__ dstChunk, 
    const uint32_t srcInChunkIndex, 
    const uint32_t dstInChunkIndex
){
#ifdef DEBUG
    if(srcChunk == dstChunk)
        throw std::invalid_argument("inArchetypeCopy(): unexpected same chunk");
#endif
    for(uint32_t i=0;i < this->nonZeroSizedTypesCount();i++) {
        uint8_t const * const src =
                (uint8_t*)srcChunk +
                this->offsets[i] +
                (this->sizeOfs[i] * srcInChunkIndex);
        uint8_t * const dst =
                (uint8_t*)dstChunk +
                this->offsets[i] +
                (this->sizeOfs[i] * dstInChunkIndex);
        memcpy(dst,src,this->sizeOfs[i]);
    }
}
void Archetype::inArchetypeCopy(
    void *chunk, 
    const uint32_t srcInChunkIndex, 
    const uint32_t dstInChunkIndex
){
#ifdef DEBUG
    if(srcInChunkIndex == dstInChunkIndex)
        throw std::invalid_argument("inArchetypeCopy(): unexpected same entity");
#endif
    for(uint32_t i=0;i < this->nonZeroSizedTypesCount();i++) {
        uint8_t const * const src =
                (uint8_t*)chunk +
                this->offsets[i] +
                (this->sizeOfs[i] * srcInChunkIndex);
        uint8_t * const dst =
                (uint8_t*)chunk +
                this->offsets[i] +
                (this->sizeOfs[i] * dstInChunkIndex);
        memcpy(dst,src,this->sizeOfs[i]);
    }
}
void Archetype::callComponentDestructor(uint32_t entityIndex) {

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
void Archetype::callComponentConstructor(uint32_t entityIndex, Entity entity) {
    uint16_t const * const typeIndex = this->realIndecies.data();
    uint32_t const * const typeOffset = this->offsets.data();
    uint16_t const * const typeSize = this->sizeOfs.data();
    uint8_t   * const chunk = (uint8_t *)this->chunksData[this->getChunkIndex(entityIndex)].memory;
    const uint32_t srcInChunkIndex = this->getInChunkIndex(entityIndex);
    uint16_t count = this->nonZeroSizedTypesCount();

    ((Entity*)(chunk+typeOffset[0]))[srcInChunkIndex] = entity;

    for(uint16_t cIndex = 1;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.constructor)
            info.constructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
