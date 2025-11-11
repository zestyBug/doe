#include "ECS/Archetype.hpp"
using namespace ECS;


mark_ptr<Archetype> Archetype::createArchetype(const_span<TypeID> types) {
    // MAGIC NUMBER
    if(unlikely(types.size() < 2 || types.size() > MaxTypePerArchetype))
        throw std::invalid_argument("createArchetype(): archetype with unexpected component count");

    uint32_t additional = 0;
    uint16_t offsets[4];
    offsets[0] =              (uint16_t)alignTo64(sizeof(Archetype),1);
    offsets[1] = offsets[0] + (uint16_t)alignTo64(sizeof(TypeID),types.size());
    offsets[2] = offsets[1] + (uint16_t)alignTo64(sizeof(uint16_t),types.size());
    offsets[3] = offsets[2] + (uint16_t)alignTo64(sizeof(uint32_t),types.size());
    additional = offsets[3] + (uint16_t)alignTo64(sizeof(uint16_t),types.size());

    mark_ptr<Archetype> archHolder{(Archetype*) allocator().allocate(sizeof(Archetype) + additional)};
    Archetype *arch = archHolder.get();

    new (arch) Archetype(types.size());

    arch->__types        = (TypeID*)  ((uint8_t*)(arch) + offsets[0]);
    arch->__realIndecies = (uint16_t*)((uint8_t*)(arch) + offsets[1]);
    arch->__offsets      = (uint32_t*)((uint8_t*)(arch) + offsets[2]);
    arch->__sizeOfs      = (uint16_t*)((uint8_t*)(arch) + offsets[3]);

    if(unlikely(types[0].value != 0))
        throw std::runtime_error("createArchetype(): getTypeInfo<Entity>().value.realIndex() must be always 0!");


    for (uint32_t i = 0;i < arch->__type_count; i++) {
        arch->__types[i] = types[i];
        arch->__realIndecies[i] = types[i].realIndex();
    }

    arch->chunksData.reserve(16);

    {
        uint16_t i = (uint16_t) arch->__type_count;
        do arch->firstTagIndex = i;
        while (i!=0 && getTypeInfo(arch->__types[--i]).size == 0);
        i++;
    }
    for (uint32_t i = 0; i < arch->__type_count; ++i)
    {
        if (i < arch->firstTagIndex){
            const auto& cType = getTypeInfo(arch->__types[i]);
            arch->__sizeOfs[i] = (uint16_t) cType.size;
        }else
            arch->__sizeOfs[i] = 0;
    }
    for (uint32_t i = 0; i < arch->__type_count; ++i)
    {
        if(arch->__types[i].isPrefab())
            arch->flags |= TypeID::prefab;
    }

    arch->chunkCapacity = std::min( 
        calculateChunkCapacity(Chunk::bufferSize,{arch->__sizeOfs,arch->__type_count}) , 
        Chunk::maximumEntitiesPerChunk
    );

    // MAGIC NUMBER
    if(unlikely(arch->chunkCapacity < 2))
        throw std::length_error("createArchetype(): LARGE COMPONENTS! X(");

    uint32_t usedBytes = Chunk::memoryOffset;
    for (uint32_t typeIndex = 0; typeIndex < arch->__type_count; typeIndex++)
    {
        uint32_t sizeOf = arch->__sizeOfs[typeIndex];
        arch->__offsets[typeIndex] = usedBytes;
        usedBytes += alignTo64(sizeOf, arch->chunkCapacity);
    }

    return archHolder;
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
        this->chunksData.emplace_back(make_align<Chunk>());
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
    uint8_t *chunkMemory = chunksData.at(chunkIndex)->memory;
    if(unlikely((chunkIndex+1) >= chunksData.size() && inChunkIndex >= lastChunkEntityCount))
            throw std::out_of_range("getComponent(): invalid entityIndex");
    return chunkMemory + this->getOffset().at(componentIndex) + (this->getSize().at(componentIndex) * inChunkIndex);
}
const void* Archetype::getComponent(uint16_t componentIndex,uint32_t entityIndex) const {
    const uint32_t inChunkIndex = getInChunkIndex(entityIndex);
    const uint32_t chunkIndex = getChunkIndex(entityIndex);
    const uint8_t *chunkMemory = chunksData.at(chunkIndex)->memory;
    if(unlikely((chunkIndex+1) >= chunksData.size() && inChunkIndex >= lastChunkEntityCount))
            throw std::out_of_range("getComponent(): invalid entityIndex");
    return chunkMemory + this->getOffset().at(componentIndex) + (this->getSize().at(componentIndex) * inChunkIndex);
}

int32_t Archetype::getIndex(TypeID t) const noexcept {
    uint16_t rIndex = t.realIndex();
    const_span<uint16_t> indecies = this->getIndex(); 
    for (uint32_t i = 0; i < indecies.size(); i++){
        if(indecies[i] == rIndex)
            return i;
        else if(indecies[i] > rIndex)
            return -1;
    }
    return -1;
}
bool Archetype::getIndecies(const_span<TypeID> rtypes,uint16_t* out) const noexcept {
    const_span<TypeID> archetypeTypes = this->getType();

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









bool Archetype::hasComponent(TypeID type) const {
    const_span<TypeID> archetypeTypes = this->getType();
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
    const_span<TypeID> archetypeTypes = this->getType();
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
    const_span<TypeID> archetypeTypes = this->getType();
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
Entity Archetype::managedRemoveEntity(uint32_t entityIndex){
    if(unlikely(this->chunksData.size() < 1))
        throw std::runtime_error("removeEntity(): empty archetype");
#ifdef DEBUG
    if(unlikely(this->lastChunkEntityCount < 1))
        throw std::runtime_error("removeEntity(): unexpected internal error");
#endif

    uint32_t lastEntityIndex = this->count() - 1;

    if(unlikely(lastEntityIndex < entityIndex))
        throw std::invalid_argument("removeEntity(): entity does not exists");

    this->lastChunkEntityCount--;

    Entity ret = Entity::null;
    if(likely(lastEntityIndex !=  entityIndex))
        ret = moveComponentValues(*this,entityIndex,lastEntityIndex);
    else
        this->callComponentDestructor(entityIndex);

    // 0 means last element on the chunk is either moved to
    // the deleted entity position or is the deleted entity itself
    // so chunk must be cleared
    if(this->lastChunkEntityCount == 0){
        this->chunksData.pop_back();
        if(this->chunksData.size() != 0)
            this->lastChunkEntityCount = this->chunkCapacity;
        this->chunksVersion.popBack();
    }
    return ret;
}
Entity Archetype::moveComponentValues(uint32_t dstIndex, uint32_t srcIndex) {
    if(dstIndex == srcIndex)
        throw std::invalid_argument("moveComponentValues(): unexpected same entity");
    Entity ret;
    const uint32_t dstChunkIndex = this->getChunkIndex(dstIndex);
    const uint32_t dstInChunkIndex = this->getInChunkIndex(dstIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const dstChunkMemory = this->chunksData.at(dstChunkIndex)->memory;

    const uint32_t srcChunkIndex = this->getChunkIndex(srcIndex);
    const uint32_t srcInChunkIndex = this->getInChunkIndex(srcIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const srcChunkMemory = this->chunksData.at(srcChunkIndex)->memory;

    for(uint32_t i=0;i < this->nonZeroSizedTypesCount();i++) {
        const auto& info = getTypeInfo(this->getType()[i]);
        if(info.destructor)
        {
            uint8_t * const ptr1 =
                dstChunkMemory +
                this->getOffset()[i] +
                (this->getSize()[i] * dstInChunkIndex);
            info.destructor(ptr1);
        }
    }

    ret = ((Entity*)(srcChunkMemory+this->getOffset()[0]))[srcInChunkIndex];

    if(srcChunkMemory == dstChunkMemory)
        this->inArchetypeCopy(srcChunkMemory, srcInChunkIndex, dstInChunkIndex);
    else
        this->inArchetypeCopy(srcChunkMemory, dstChunkMemory, srcInChunkIndex, dstInChunkIndex);
    return ret;
}
Entity Archetype::copyComponentValues(uint32_t dstIndex, uint32_t srcIndex) {
    Entity ret;
    const uint32_t dstChunkIndex = this->getChunkIndex(dstIndex);
    const uint32_t dstInChunkIndex = this->getInChunkIndex(dstIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const dstChunkMemory = this->chunksData.at(dstChunkIndex)->memory;

    const uint32_t srcChunkIndex = this->getChunkIndex(srcIndex);
    const uint32_t srcInChunkIndex = this->getInChunkIndex(srcIndex);
    // im not sure that it wont break, so a at() will do the job
    uint8_t * const srcChunkMemory = this->chunksData.at(srcChunkIndex)->memory;

    ret = ((Entity*)(srcChunkMemory+this->getOffset()[0]))[srcInChunkIndex];

    if(srcChunkMemory == dstChunkMemory)
        this->inArchetypeCopy(srcChunkMemory, srcInChunkIndex, dstInChunkIndex);
    else
        this->inArchetypeCopy(srcChunkMemory, dstChunkMemory, srcInChunkIndex, dstInChunkIndex);

    return ret;
}
Entity Archetype::moveComponentValues(Archetype& srcArchetype, uint32_t dstIndex, uint32_t srcIndex){
    if(this == &srcArchetype)
        throw std::invalid_argument("moveComponentValues(): unexpected same archetype");
    Entity ret;
    uint16_t const * const srcTypeIndex = srcArchetype.getIndex().data();
    uint32_t const * const srcTypeOffset = srcArchetype.getOffset().data();
    uint16_t const * const srcTypeSize = srcArchetype.getSize().data();
    uint8_t   * const srcChunk = srcArchetype.chunksData[srcArchetype.getChunkIndex(srcIndex)]->memory;
    const uint32_t srcInChunkIndex = srcArchetype.getInChunkIndex(srcIndex);

    uint16_t const *const dstTypeIndex = this->getIndex().data();
    uint32_t const *const dstTypeOffset = this->getOffset().data();
    uint16_t const *const dstTypeSize = this->getSize().data();
    uint8_t * const dstChunk = this->chunksData[this->getChunkIndex(dstIndex)]->memory;
    const uint32_t dstInChunkIndex = this->getInChunkIndex(dstIndex);


    const uint16_t srcCount = srcArchetype.nonZeroSizedTypesCount();
    const uint16_t dstCount = this->nonZeroSizedTypesCount();

    uint16_t srcI = 0,
    dstI = 0;
    {
        // for advanced cache friendly-ness

        uint16_t toBeDeletedCount=0;
        uint16_t toBeInitCount=0;
        uint16_t toBeDeletedArray[srcCount];
        uint16_t toBeInitArray[dstCount];

        ret = ((Entity*)(srcChunk+srcTypeOffset[0]))[srcInChunkIndex];
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
        for(;dstI < dstCount;) {
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
    if(srcChunk == dstChunk)
        throw std::invalid_argument("inArchetypeCopy(): unexpected same chunk");
    auto offsets{this->getOffset()};
    auto sizes{this->getSize()};
    for(uint32_t i=0;i < this->nonZeroSizedTypesCount();i++) {
        uint8_t const * const src =
                (uint8_t*)srcChunk +
                offsets[i] +
                (sizes[i] * srcInChunkIndex);
        uint8_t * const dst =
                (uint8_t*)dstChunk +
                offsets[i] +
                (sizes[i] * dstInChunkIndex);
        memcpy(dst,src,sizes[i]);
    }
}
void Archetype::inArchetypeCopy(
    void *chunk, 
    const uint32_t srcInChunkIndex, 
    const uint32_t dstInChunkIndex
){
    if(srcInChunkIndex == dstInChunkIndex)
        throw std::invalid_argument("inArchetypeCopy(): unexpected same entity");
    auto offsets{this->getOffset()};
    auto sizes{this->getSize()};
    for(uint32_t i=0;i < this->nonZeroSizedTypesCount();i++) {
        uint8_t const * const src =
                (uint8_t*)chunk +
                offsets[i] +
                (sizes[i] * srcInChunkIndex);
        uint8_t * const dst =
                (uint8_t*)chunk +
                offsets[i] +
                (sizes[i] * dstInChunkIndex);
        memcpy(dst,src,sizes[i]);
    }
}
void Archetype::callComponentDestructor(uint32_t entityIndex) {

    uint16_t const * const typeIndex = this->getIndex().data();
    uint32_t const * const typeOffset = this->getOffset().data();
    uint16_t const * const typeSize = this->getSize().data();
    uint8_t * const chunk = this->chunksData[this->getChunkIndex(entityIndex)]->memory;
    const uint32_t srcInChunkIndex = this->getInChunkIndex(entityIndex);
    uint16_t count = this->nonZeroSizedTypesCount();

    for(uint16_t cIndex = 0;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.destructor)
            info.destructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
void Archetype::callComponentConstructor(uint32_t entityIndex, Entity entity) {
    uint16_t const * const typeIndex = this->getIndex().data();
    uint32_t const * const typeOffset = this->getOffset().data();
    uint16_t const * const typeSize = this->getSize().data();
    uint8_t * const chunk = this->chunksData[this->getChunkIndex(entityIndex)]->memory;
    const uint32_t srcInChunkIndex = this->getInChunkIndex(entityIndex);
    uint16_t count = this->nonZeroSizedTypesCount();

    ((Entity*)(chunk+typeOffset[0]))[srcInChunkIndex] = entity;

    for(uint16_t cIndex = 1;cIndex < count;++cIndex) {
        auto& info = getTypeInfo(typeIndex[cIndex]);
        if(info.constructor)
            info.constructor(chunk + typeOffset[cIndex] + typeSize[cIndex] * srcInChunkIndex);
    }
}
