#include "ECS/Archetype.hpp"
using namespace DOTS;


Archetype* Archetype::createArchetype(span<TypeIndex> types) {
    const size_t typesCount = types.size() + 1;
    if(typesCount < 1 || typesCount > 0xFFF)
        throw std::invalid_argument("createArchetype(): archetype with unexpected component count");

    uint32_t additional = 0;
    uint16_t offsets[4];
    offsets[0] = getComponentArraySize(sizeof(Archetype),1);
    offsets[1] = offsets[0] + getComponentArraySize(sizeof(TypeIndex),typesCount);
    offsets[2] = offsets[1] + getComponentArraySize(sizeof(uint16_t),typesCount);
    offsets[3] = offsets[2] + getComponentArraySize(sizeof(uint32_t),typesCount);
    additional = offsets[3] + getComponentArraySize(sizeof(uint16_t),typesCount);


    Archetype *arch = (Archetype*) allocator().allocate(sizeof(Archetype) + additional);

    new (arch) Archetype();

    arch->types = span<TypeIndex>{(TypeIndex*)((uint8_t*)(arch) + offsets[0]),typesCount};
    arch->realIndecies = span<uint16_t>{(uint16_t*)((uint8_t*)(arch) + offsets[1]),typesCount};
    arch->offsets = span<uint32_t>{(uint32_t*)((uint8_t*)(arch) + offsets[2]),typesCount};
    arch->sizeOfs = span<uint16_t>{(uint16_t*)((uint8_t*)(arch) + offsets[3]),typesCount};

    // TODO: this value must be always 0
    arch->types[0] = getTypeInfo<Entity>().value;
    if(arch->types[0].realIndex() != 0)
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

    arch->chunkCapacity = std::min( calculateChunkCapacity(Chunk::_BufferSize,arch->sizeOfs) , maxEntityPerChunk);

    if(arch->chunkCapacity < 2)
        throw std::length_error("createArchetype(): LARGE COMPONENTS! X(");

    uint32_t usedBytes = Chunk::_BufferOffset;
    for (uint32_t typeIndex = 0; typeIndex < typesCount; typeIndex++)
    {
        uint32_t sizeOf = arch->sizeOfs[typeIndex];

        // align usedBytes upwards (eating into alignExtraSpace) so that
        // this component actually starts at its required alignment.
        // Assumption is that the start of the entire data segment is at the
        // maximum possible alignment.
        arch->offsets[typeIndex] = usedBytes;
        usedBytes += getComponentArraySize(sizeOf, arch->chunkCapacity);
    }

    return arch;
}

uint32_t Archetype::createEntity() {
    uint32_t res = 0;
    if(lastChunkEntityCount == 0 && !this->chunksData.empty())
        throw std::runtime_error("createEntity(): intrnal error!");
    if(this->chunksData.empty() || lastChunkEntityCount >= this->chunkCapacity){
        lastChunkEntityCount=1;
        this->chunksData.emplace_back().memory = allocator().allocate(Chunk::_ChunkSize);
        chunks.add(0);
    }else
        ++lastChunkEntityCount;

    res = this->count() - 1;

    if(res >= nullEntityIndex)
        throw std::out_of_range("entity count limit reached");
    return res;
}

uint32_t Archetype::removeEntity(uint32_t entityIndex){
    if(this->chunksData.size() < 1)
        throw std::runtime_error("removeEntity(): empty archetype");
    if(this->lastChunkEntityCount < 1)
        throw std::runtime_error("removeEntity(): unexpected internal error");

    uint32_t lastEntityIndex = count() - 1;

    if(lastEntityIndex < entityIndex)
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
    if((chunkIndex+1) >= chunksData.size())
        if(inChunkIndex >= lastChunkEntityCount)
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
