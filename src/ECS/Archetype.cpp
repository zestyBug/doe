#include "ECS/Archetype.hpp"
#include "ECS/EntityComponentStore.hpp"
using namespace ECS;

void Archetype::addToChunkList(Chunk* chunk, SharedComponentValues sharedComponentIndices, uint32_t changeVersion) {
    if(chunk == nullptr)
        throw std::invalid_argument("addToChunkList(): invalid chunk");
    chunk->listIndex = chunks._count;
    chunks.add(chunk, sharedComponentIndices, changeVersion);
}
void Archetype::removeFromChunkList(Chunk* chunk){
    if(chunk == nullptr)
        throw std::invalid_argument("removeFromChunkList(): invalid chunk");
    int32_t chunkListIndex = chunk->listIndex;
    if(chunkListIndex < 0)
        throw std::invalid_argument("removeFromChunkList(): invalid chunk");
    chunks.removeAtSwapBack(chunkListIndex);
    Chunk* chunkThatMoved = chunks[chunkListIndex];
    chunkThatMoved->listIndex = chunkListIndex;
}
void Archetype::addToChunkListWithEmptySlots(Chunk* chunk){
    chunk->listWithEmptySlotsIndex = (uint32_t)chunksWithEmptySlots.size();
    chunksWithEmptySlots.push_back(chunk);
}
void Archetype::removeFromChunkListWithEmptySlots(Chunk* chunk){
    if(chunk == nullptr)
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    int32_t index = chunk->listWithEmptySlotsIndex;
    if((uint32_t)index >= chunksWithEmptySlots.size() || 0 > index)
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    if(chunk != chunksWithEmptySlots[index])
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    Chunk* lastChunk = chunksWithEmptySlots.back();
    if (chunk != lastChunk)
    {
        lastChunk->listWithEmptySlotsIndex = index;
        chunksWithEmptySlots[index] = lastChunk;
    }
    chunksWithEmptySlots.pop_back();
}
void Archetype::emptySlotTrackingRemoveChunk(Chunk* chunk){
    if(!chunk || this != chunk->archetype)
        throw std::invalid_argument("emptySlotTrackingRemoveChunk(): invalid chunk");
    if (numSharedComponents() == 0)
        removeFromChunkListWithEmptySlots(chunk);
    else
        freeChunksBySharedComponents.remove(chunk);
}
void Archetype::emptySlotTrackingAddChunk(Chunk* chunk){
    if(!chunk || this != chunk->archetype)
        throw std::invalid_argument("emptySlotTrackingAddChunk(): invalid chunk");
    if (numSharedComponents() == 0)
        addToChunkListWithEmptySlots(chunk);
    else
        freeChunksBySharedComponents.add(chunk);
}
Chunk* Archetype::getExistingChunkWithEmptySlots(SharedComponentValues sharedComponentValues){
    if (numSharedComponents() == 0)
    {
        if (!chunksWithEmptySlots.empty())
        {
            Chunk* chunk = chunksWithEmptySlots.back();
            if(chunk->entityCount >= this->chunkCapacity)
                throw std::runtime_error("getExistingChunkWithEmptySlots(): invalid chunk");
            return chunk;
        }
        return nullptr;
    }
    // note: will be ChunkIndex.Null if none available.
    return freeChunksBySharedComponents.tryGet(sharedComponentValues, numSharedComponents());
}




int32_t Archetype::getIndexInTypeArray(TypeID type) const
{
    TypeID *types = this->_types;
    TypeID *types_end = types + this->typeCount;
    for (;types!=types_end;++types){
        if (type == *types)
            return (int32_t)(types_end - types);
        else if(type < *types)
            return -1;
    }
    return -1;
}
int32_t Archetype::getNextIndexInTypeArray(TypeID type, int32_t lastTypeIndexInTypeArray) const
{
    if(lastTypeIndexInTypeArray < 0 && lastTypeIndexInTypeArray >= this->typeCount);
        throw std::out_of_range("getNextIndexInTypeArray(): invalid last index");
    TypeID *types = this->_types + lastTypeIndexInTypeArray;
    TypeID *types_end = types + this->typeCount;
    for (;types!=types_end;++types) {
        if (type == *types)
            return (int32_t)(types_end - types);
        else if(type < *types)
            return -1;
    }
    return -1;
}
void Archetype::releaseChunk(Chunk* chunk)
{
    if(chunk == nullptr)
        throw std::invalid_argument("addToChunkList(): invalid chunk");
    EntityComponentStore* entityComponentStore = this->entityComponentStore;
    // Remove references to shared components
    if (this->numSharedComponents() > 0)
    {
        SharedComponentValues sharedComponentValueArray = this->chunks.getSharedComponentValues(chunk->listIndex);
        for (uint32_t i = 0; i < this->numSharedComponents(); ++i)
        {
            SharedComponentIndex sharedComponentIndex = sharedComponentValueArray[i];
            entityComponentStore->sharedComponents.removeReference(sharedComponentIndex);
        }
    }
    // this chunk is going away, so it shouldn't be in the empty slot list.
    if (chunk->entityCount < this->chunkCapacity)
        this->emptySlotTrackingRemoveChunk(chunk);
    this->removeFromChunkList(chunk);
    entityComponentStore->chunks.freeChunk(chunk->index);
}
void Archetype::setChunkCount(Chunk* chunk, uint32_t newCount)
{
    if(chunk->entityCount == newCount)
        throw std::invalid_argument("setChunkCount(): no change detected");
    // Chunk released to empty chunk pool
    if (newCount == 0)
    {
        releaseChunk(chunk);
        return;
    }
    uint32_t capacity = this->chunkCapacity;
    // Chunk is now full
    if (newCount == capacity)
    {
        // this chunk no longer has empty slots, so it shouldn't be in the empty slot list.
        this->emptySlotTrackingRemoveChunk(chunk);
    }
    // Chunk is no longer full
    else if (chunk->entityCount == capacity)
    {
        if(newCount >= chunk->entityCount)
            throw std::runtime_error("setChunkCount(): ");
        this->emptySlotTrackingAddChunk(chunk);
    }
    chunk->entityCount = newCount;
}
void Archetype::initializeComponents(Chunk* chunk, uint32_t dstIndex, uint32_t count) {
    uint32_t *offsets = this->_offsets;
    uint16_t *sizeOfs = this->_sizeOfs;
    uint8_t  *dstBuffer = chunk->buffer;
    uint32_t  typesCount = this->typeCount;
    TypeID   *types = this->_types;
    for (uint32_t t = 1; t != typesCount; t++)
    {
        uint32_t sizeOf = sizeOfs[t];
        uint8_t *dst = dstBuffer + (offsets[t] + sizeOf * dstIndex);
        memset(dst, 0, sizeOf*count);
    }
}
uint32_t Archetype::allocateIntoChunk(Chunk* chunk, uint32_t count, uint32_t& outIndex)
{
    outIndex = chunk->entityCount;
    uint32_t allocatedCount = std::min(chunkCapacity - outIndex, count);
    setChunkCount(chunk, outIndex + allocatedCount);
    this->entityCount += allocatedCount;
    return allocatedCount;
}
uint32_t Archetype::allocate(Chunk* chunk, uint32_t count, Entity *entities)
{
    EntityComponentStore* entityComponentStore = this->entityComponentStore;
    Version globalSystemVersion = entityComponentStore->getGlobalSystemVersion();
    uint32_t allocatedIndex;
    uint32_t allocatedCount = this->allocateIntoChunk(chunk, count, allocatedIndex);
    entityComponentStore->allocateEntities(this, chunk, allocatedIndex, allocatedCount, entities);
    initializeComponents(chunk, allocatedIndex, allocatedCount);

    // Add Entities in Chunk. ChangeVersion:Yes OrderVersion:Yes
    this->chunks.setOrderVersion(chunk->listIndex, globalSystemVersion);
    this->chunks.setAllChangeVersion(chunk->listIndex, globalSystemVersion);
    entityComponentStore->incrementComponentTypeOrderVersion(this);

    return allocatedCount;
}
void Archetype::deallocate(Chunk *chunk)
{
    if(chunk == nullptr)
        throw std::invalid_argument("deallocate(): invalid batch");
    this->deallocate(EntityBatchInChunk{ .chunk = chunk, .startIndex = 0, .count = chunk->entityCount });
}
void Archetype::deallocate(EntityBatchInChunk batch)
{
    if(batch.chunk == nullptr || batch.chunk->entityCount < (batch.count + batch.startIndex))
        throw std::invalid_argument("deallocate(): invalid batch");

    EntityComponentStore *entityComponentStore = this->entityComponentStore;

    entityComponentStore->deallocateDataEntitiesInChunk(batch);
    SharedComponentValues sharedComponentValues = this->chunks.getSharedComponentValues(batch.chunk->listIndex);
    entityComponentStore->incrementComponentOrderVersion(this, sharedComponentValues);

    // Remove Entities in Chunk. ChangeVersion:No OrderVersion:Yes
    this->chunks.setOrderVersion(batch.chunk->listIndex, entityComponentStore->getGlobalSystemVersion());
    entityComponentStore->incrementComponentTypeOrderVersion(this);
    //IncrementComponentTypeOrderVersion

    this->entityCount -= batch.count;
    uint32_t newChunkEntityCount = batch.chunk->entityCount - batch.count;
    this->setChunkCount(batch.chunk, newChunkEntityCount);
}
void Archetype::remove(EntityBatchInChunk batch)
{
    if(batch.chunk == nullptr)
        throw std::invalid_argument("remove(): invalid batch");
    
    Archetype *archetype = batch.chunk->archetype;
    EntityComponentStore *entityComponentStore = archetype->entityComponentStore;
    const uint32_t chunk_Count = batch.chunk->entityCount;
    // Fill in moved component data from the end.
    const uint32_t srcTailIndex = batch.startIndex + batch.count;
    if(chunk_Count < srcTailIndex)
        throw std::invalid_argument("remove(): invalid batch");
    const uint32_t srcTailCount = chunk_Count - srcTailIndex;
    const uint32_t fillCount = std::min(batch.count, srcTailCount);

    if (fillCount > 0)
    {
        const uint32_t fillStartIndex = chunk_Count - fillCount;
        Archetype::copy(batch.chunk, fillStartIndex, batch.chunk, batch.startIndex, fillCount);
        const uint32_t sizeOf = archetype->_sizeOfs[0];
        Entity *fillEntities = (Entity*)batch.chunk->buffer + (sizeOf * batch.startIndex);
        for (int i = 0; i < fillCount; i++)
            entityComponentStore->setEntityInChunk(fillEntities[i], { batch.chunk, batch.startIndex + i });
    }

    archetype->chunks.setOrderVersion(batch.chunk->listIndex, entityComponentStore->getGlobalSystemVersion());
    entityComponentStore->incrementComponentTypeOrderVersion(archetype);
    SharedComponentValues sharedComponentValues = archetype->chunks.getSharedComponentValues(batch.chunk->listIndex);
    entityComponentStore->incrementComponentOrderVersion(archetype, sharedComponentValues);

    int newChunkEntityCount = batch.chunk->entityCount - batch.count;
    archetype->setChunkCount(batch.chunk, newChunkEntityCount);
    archetype->entityCount -= batch.count;
}
void Archetype::copy(Chunk *srcChunk, uint32_t srcIndex, Chunk *dstChunk, uint32_t dstIndex, uint32_t count)
{
    Archetype *arch = srcChunk->archetype;

    if(arch != dstChunk->archetype)
        throw std::invalid_argument("copy(): the archetypes do not match");

    uint8_t *srcBuffer = srcChunk->buffer;
    uint8_t *dstBuffer = dstChunk->buffer;
    uint32_t *offsets = arch->_offsets;
    uint16_t *sizeOfs = arch->_sizeOfs;
    uint32_t typesCount = arch->typeCount;

    for (uint32_t t = 0; t < typesCount; t++)
    {
        const uint32_t offset = offsets[t];
        const uint32_t sizeOf = sizeOfs[t];
        void *src = srcBuffer + (offset + sizeOf * srcIndex);
        void *dst = dstBuffer + (offset + sizeOf * dstIndex);

        memcpy(dst, src, sizeOf * count);
    }
}
void Archetype::copyComponents(Chunk *srcChunk, uint32_t srcIndex, Chunk *dstChunk, uint32_t dstIndex, uint32_t count, uint32_t dstGlobalSystemVersion)
{
    if(!areLayoutCompatible(dstChunk->archetype,srcChunk->archetype));
        throw std::invalid_argument("copyComponents(): incompatible archetypes layout");

    Archetype *dstArch   = dstChunk->archetype;
    uint8_t  *srcBuffer  = srcChunk->buffer;
    uint8_t  *dstBuffer  = dstChunk->buffer;
    uint32_t *offsets    = dstArch->_offsets;
    uint16_t *sizeOfs    = dstArch->_sizeOfs;
    uint32_t  typesCount = dstArch->typeCount;
    int32_t dstChunkListIndex = dstChunk->listIndex;

    for (uint32_t t = 1; t < typesCount; t++) // Only copy component data, not Entity
    {
        const uint32_t offset = offsets[t];
        const uint32_t sizeOf = sizeOfs[t];
        void *src = srcBuffer + (offset + sizeOf * srcIndex);
        void *dst = dstBuffer + (offset + sizeOf * dstIndex);
        dstArch->chunks.setChangeVersion(t, dstChunkListIndex, dstGlobalSystemVersion);
        memcpy(dst, src, sizeOf * count);
    }
}
bool Archetype::areLayoutCompatible(Archetype* a, Archetype* b)
{
    if(a == nullptr || b == nullptr)
        return false;
    if(a != b)
    {
        // quick check
        if(a->chunkCapacity != b->chunkCapacity)
            return false;
        uint32_t typeCount = a->numNonZeroSizedTypes();
        if(typeCount != b->numNonZeroSizedTypes())
            return false;
        for(uint32_t i = 0; i < typeCount; ++i){
            if(a->_types[i] != b->_types[i])
                return false;
        }
    }
    return true;
}

void Archetype::setSharedComponentDataIndex(Entity entity, SharedComponentValues sharedComponentValues, TypeID typeIndex)
{
    EntityComponentStore *entityComponentStore = this->entityComponentStore;
    Chunk *chunk = entityComponentStore->getChunk(entity);
    // chunk should still be in the same archetype
    if(this != chunk->archetype)
        throw std::invalid_argument("setSharedComponentDataIndex(): invalid archetype");
    int32_t indexInTypeArray = this->getIndexInTypeArray(typeIndex);
    if(indexInTypeArray < 0)
        throw std::invalid_argument("setSharedComponentDataIndex(): type not found");
    Version globalSystemVersion = entityComponentStore->getGlobalSystemVersion();

    entityComponentStore->move(entity, this, sharedComponentValues);

    this->chunks.setChangeVersion(indexInTypeArray, chunk->listIndex, globalSystemVersion);
}

void Archetype::setSharedComponentDataIndex(Chunk *chunk, SharedComponentValues sharedComponentValues, TypeID typeIndex)
{
    EntityComponentStore *entityComponentStore = this->entityComponentStore;
    // chunk should still be in the same archetype
    if(this != chunk->archetype)
        throw std::invalid_argument("setSharedComponentDataIndex(): invalid archetype");
    int32_t indexInTypeArray = this->getIndexInTypeArray(typeIndex);
    if(indexInTypeArray < 0)
        throw std::invalid_argument("setSharedComponentDataIndex(): type not found");
    Version globalSystemVersion = entityComponentStore->getGlobalSystemVersion();

    entityComponentStore->move(chunk, this, sharedComponentValues);

    this->chunks.setChangeVersion(indexInTypeArray, chunk->listIndex, globalSystemVersion);
}

void Archetype::setSharedComponentDataIndex(EntityBatchInChunk batch, SharedComponentValues sharedComponentValues, TypeID type)
{
    EntityComponentStore *entityComponentStore = this->entityComponentStore;
    entityComponentStore->moveAndSetChangeVersion(batch, batch.chunk->archetype, sharedComponentValues, type);
}

const uint8_t* Archetype::getComponentDataWithTypeRO(Chunk *chunk, uint32_t baseEntityIndex, TypeID type) const
{
    int32_t indexInTypeArray = this->getIndexInTypeArray(type);
    if(indexInTypeArray < 0)
        throw std::invalid_argument("getComponentDataWithTypeRO(): type not fount");
    if(chunk->entityCount <= baseEntityIndex)
        throw std::out_of_range("getComponentDataWithTypeRO(): invalid entity index");
    uint32_t offset = this->_offsets[indexInTypeArray];
    uint32_t sizeOf = this->_sizeOfs[indexInTypeArray];

    return chunk->buffer + (offset + sizeOf * baseEntityIndex);
}
const uint8_t* Archetype::getComponentDataRO(Chunk *chunk, uint32_t baseEntityIndex, uint32_t indexInTypeArray) const
{
    if(indexInTypeArray < 0)
        throw std::invalid_argument("getComponentDataWithTypeRO(): type not fount");
    if(chunk->entityCount <= baseEntityIndex)
        throw std::out_of_range("getComponentDataWithTypeRO(): invalid entity index");
    uint32_t offset = this->_offsets[indexInTypeArray];
    uint32_t sizeOf = this->_sizeOfs[indexInTypeArray];

    return chunk->buffer + (offset + sizeOf * baseEntityIndex);
}
uint8_t* Archetype::getComponentDataWithTypeRW(Chunk *chunk, uint32_t baseEntityIndex, TypeID type, Version globalSystemVersion)
{
    int32_t indexInTypeArray = this->getIndexInTypeArray(type);
    if(indexInTypeArray < 0)
        throw std::invalid_argument("getComponentDataWithTypeRO(): type not fount");
    if(chunk->entityCount <= baseEntityIndex)
        throw std::out_of_range("getComponentDataWithTypeRO(): invalid entity index");
    uint32_t offset = this->_offsets[indexInTypeArray];
    uint32_t sizeOf = this->_sizeOfs[indexInTypeArray];

    // Write Component to Chunk. ChangeVersion:Yes OrderVersion:No
    this->chunks.setChangeVersion(indexInTypeArray, chunk->listIndex, globalSystemVersion);

    return chunk->buffer + (offset + sizeOf * baseEntityIndex);
}
uint8_t* Archetype::getComponentDataRW(Chunk *chunk, uint32_t baseEntityIndex, uint32_t indexInTypeArray, Version globalSystemVersion)
{
    if(indexInTypeArray < 0)
        throw std::invalid_argument("getComponentDataWithTypeRO(): type not fount");
    if(chunk->entityCount <= baseEntityIndex)
        throw std::out_of_range("getComponentDataWithTypeRO(): invalid entity index");
    uint32_t offset = this->_offsets[indexInTypeArray];
    uint32_t sizeOf = this->_sizeOfs[indexInTypeArray];

    // Write Component to Chunk. ChangeVersion:Yes OrderVersion:No
    this->chunks.setChangeVersion(indexInTypeArray, chunk->listIndex, globalSystemVersion);

    return chunk->buffer + (offset + sizeOf * baseEntityIndex);
}
void Archetype::addEmptyChunk(Chunk *chunk, SharedComponentValues sharedComponentValues)
{
    EntityComponentStore *entityComponentStore = this->entityComponentStore;
    Version globalSystemVersion = entityComponentStore->getGlobalSystemVersion();

    entityComponentStore->setArchetype(chunk, this);
    chunk->entityCount = 0;
    uint32_t numSharedComponents = this->numSharedComponents();
    if (numSharedComponents > 0)
    {
        for (uint32_t i = 0; i < numSharedComponents; ++i)
        {
            SharedComponentIndex sharedComponentIndex = sharedComponentValues[i];
            entityComponentStore->sharedComponents.addReference(sharedComponentIndex);
        }
    }

    this->addToChunkList(chunk, sharedComponentValues, globalSystemVersion);
    if(this->chunks.empty())
        throw std::runtime_error("addEmptyChunk(): internal error");

    // Chunk can't be locked at at construction time
    this->emptySlotTrackingAddChunk(chunk);
    if(numSharedComponents == 0){
        if(this->chunksWithEmptySlots.empty())
            throw std::runtime_error("addEmptyChunk(): internal error");
    }else{
        if(this->freeChunksBySharedComponents.tryGet(
                this->chunks.getSharedComponentValues(chunk->listIndex),
                numSharedComponents
            ) == nullptr)
            throw std::runtime_error("addEmptyChunk(): internal error");
    }
}
void Archetype::clone(Archetype *srcArchetype, EntityBatchInChunk srcBatch, Archetype *dstArchetype, Chunk *dstChunk)
{
    EntityComponentStore *entityComponentStore = dstArchetype->entityComponentStore;
    Version globalSystemVersion = entityComponentStore->getGlobalSystemVersion();

    // Note (srcArchetype == dstArchetype) is valid
    // Archetypes can the the same, but chunks still differ because filter is different (e.g. shared component)

    bool dstValidExistingVersions = dstChunk->entityCount != 0;
    uint32_t dstChunkIndex;
    uint32_t dstCount = dstArchetype->allocateIntoChunk(dstChunk, srcBatch.count, dstChunkIndex);
    if(dstCount != srcBatch.count)
        throw std::runtime_error("clone(): unable to allocate into the chunk");

    Archetype::convert(srcArchetype, srcBatch.chunk, srcBatch.startIndex, dstArchetype, dstChunk, dstChunkIndex, dstCount);

    Entity *dstEntities = (Entity*)dstChunk->buffer + dstChunkIndex;
    for (int i = 0; i < dstCount; i++)
        entityComponentStore->setEntityInChunk(dstEntities[i], { dstChunk, dstChunkIndex + i });

    Archetype::cloneChangeVersions(srcArchetype, srcBatch.chunk->listIndex , dstArchetype, dstChunk->listIndex, dstValidExistingVersions);

    dstArchetype->chunks.setOrderVersion(dstChunk->listIndex, globalSystemVersion);
    entityComponentStore->incrementComponentTypeOrderVersion(dstArchetype);
    SharedComponentValues dstSharedComponentValues = dstArchetype->chunks.getSharedComponentValues(dstChunk->listIndex);
    entityComponentStore->incrementComponentOrderVersion(dstArchetype, dstSharedComponentValues);

    // Cannot DestroyEntities unless CleanupComplete on the entity chunk.
    entityComponentStore->destroyEntities({dstEntities, dstCount});
}
void Archetype::convert(Archetype *srcArchetype, Chunk *srcChunk, uint32_t srcIndex, Archetype *dstArchetype, Chunk *dstChunk, uint32_t dstIndex, uint32_t count)
{
    if(srcChunk == dstChunk)
        throw std::invalid_argument("convert(): logic error, same chunk conversion");
    if(srcArchetype == nullptr || dstArchetype == nullptr || srcChunk == nullptr || dstArchetype == nullptr)
        throw std::invalid_argument("convert(): nullptr");
    EntityComponentStore *entityComponentStore = dstArchetype->entityComponentStore;

    // Process non-zero-sized types
    uint32_t srcI = srcArchetype->numNonZeroSizedTypes() - 1;
    uint32_t dstI = dstArchetype->numNonZeroSizedTypes() - 1;
    uint32_t srcFirstManagedComponent = srcArchetype->firstManagedComponent;

    const TypeID *srcTypes = srcArchetype->_types;
    const TypeID *dstTypes = dstArchetype->_types;
    const uint16_t *srcSizeOfs = srcArchetype->_sizeOfs;
    const uint16_t *dstSizeOfs = dstArchetype->_sizeOfs;
    const uint32_t *srcOffsets = srcArchetype->_offsets;
    const uint32_t *dstOffsets = dstArchetype->_offsets;
    uint8_t *srcChunkBuffer = srcChunk->buffer;
    uint8_t *dstChunkBuffer = dstChunk->buffer;

    uint32_t sourceTypesToDealloc[srcI + 1];
    uint32_t sourceTypesToDeallocCount = 0;

    while (dstI >= 0){
        TypeID srcType = srcTypes[srcI];
        TypeID dstType = dstTypes[dstI];

        if (srcType > dstType){
            //Type in source is not moved so deallocate it
            sourceTypesToDealloc[sourceTypesToDeallocCount++] = srcI;
            --srcI;
            continue;
        }

        uint32_t srcStride = srcSizeOfs[srcI];
        uint32_t dstStride = dstSizeOfs[dstI];
        uint8_t *src = srcChunkBuffer + srcOffsets[srcI] + srcIndex * srcStride;
        uint8_t *dst = dstChunkBuffer + dstOffsets[dstI] + dstIndex * dstStride;

        if (srcType == dstType){
            // Component exists in both src and dst archetypes; copy current value.
            memcpy(dst, src, count * srcStride);
            --srcI;
            --dstI;
        }else{
            // Component is in dst but not source. Clear values to default.
            memset(dst, 0, count * dstStride);
            --dstI;
        }
    }

    if (sourceTypesToDeallocCount == 0)
        return;

    sourceTypesToDealloc[sourceTypesToDeallocCount] = 0;

    uint32_t iDealloc = 0;
    if (sourceTypesToDealloc[iDealloc] >= srcFirstManagedComponent)
    {
        do
        {
            srcI = sourceTypesToDealloc[iDealloc];
            uint32_t srcStride = srcSizeOfs[srcI];
            TypeManager::DefaultFunction dFunc = TypeManager::GetTypeInfo(srcTypes[srcI]).defaultDestruct;
            uint8_t *src = srcChunkBuffer + srcOffsets[srcI] + srcIndex * srcStride;
            for (uint32_t i = 0; i < count; i++)
            {
                dFunc(src);
                src += srcStride;
            }
        }
        while ((sourceTypesToDealloc[++iDealloc] >= srcFirstManagedComponent));
    }
}
void Archetype::cloneChangeVersions(Archetype* srcArchetype, int32_t chunkIndexInSrcArchetype, Archetype* dstArchetype, int32_t chunkIndexInDstArchetype, bool dstValidExistingVersions)
{
    TypeID *dstTypes = dstArchetype->_types;
    TypeID *srcTypes = srcArchetype->_types;
    Version dstGlobalSystemVersion = dstArchetype->entityComponentStore->getGlobalSystemVersion();
    Version srcGlobalSystemVersion = dstArchetype->entityComponentStore->getGlobalSystemVersion();

    for (int isrcType = srcArchetype->typeCount - 1, idstType = dstArchetype->typeCount - 1;
            idstType >= 0;
            --idstType)
    {
        TypeID dstType = dstTypes[idstType];
        TypeID srcType = srcTypes[idstType];
        while (srcType > dstType)
            --isrcType;

        Version version = dstGlobalSystemVersion;

        // select "newer" version relative to dst EntityComponentStore GlobalSystemVersion
        if (srcType == dstType)
        {
            Version srcVersion = srcArchetype->chunks.getChangeVersion(isrcType, chunkIndexInSrcArchetype);
            if (dstValidExistingVersions)
            {
                Version dstVersion = dstArchetype->chunks.getChangeVersion(idstType, chunkIndexInDstArchetype);

                Version srcVersionSinceChange = srcGlobalSystemVersion - srcVersion;
                Version dstVersionSinceChange = dstGlobalSystemVersion - dstVersion;

                if (dstVersionSinceChange < srcVersionSinceChange)
                    version = dstVersion;
                else
                    version = dstGlobalSystemVersion - srcVersionSinceChange;
            }
            else
            {
                version = srcVersion;
            }
        }

        dstArchetype->chunks.setChangeVersion(idstType, chunkIndexInDstArchetype, version);
    }
}
void Archetype::changeArchetypeInPlace(Archetype* srcArchetype, Chunk *srcChunk, Archetype* dstArchetype, SharedComponentValues dstSharedComponentValues)
{
    EntityComponentStore *entityComponentStore = dstArchetype->entityComponentStore;
    if(areLayoutCompatible(srcArchetype, dstArchetype))
        throw std::invalid_argument("invalid arguement");

    SharedComponentValues srcSharedComponentValues = srcArchetype->chunks.getSharedComponentValues(srcChunk->listIndex);

    bool fixupSharedComponentReferences = 
        (srcArchetype->numSharedComponents() > 0) || 
        (dstArchetype->numSharedComponents() > 0);
    if (fixupSharedComponentReferences)
    {
        uint32_t srcCount = srcArchetype->numSharedComponents();
        uint32_t dstCount = dstArchetype->numSharedComponents();
        uint32_t srcFirstShared = srcArchetype->firstSharedComponent;
        uint32_t dstFirstShared = dstArchetype->firstSharedComponent;

        uint32_t o = 0;
        uint32_t n = 0;

        for (; n < dstCount && o < srcCount;)
        {
            uint32_t srcType = srcArchetype->_types[o + srcFirstShared].index();
            uint32_t dstType = dstArchetype->_types[n + dstFirstShared].index();
            if (srcType == dstType)
            {
                SharedComponentIndex srcSharedComponentDataIndex = srcSharedComponentValues[o];
                SharedComponentIndex dstSharedComponentDataIndex = dstSharedComponentValues[n];
                if (srcSharedComponentDataIndex != dstSharedComponentDataIndex)
                {
                    entityComponentStore->sharedComponents.removeReference(srcSharedComponentDataIndex);
                    entityComponentStore->sharedComponents.addReference(dstSharedComponentDataIndex);
                }
                n++;
                o++;
            }
            else if (dstType > srcType) // removed from dstArchetype
            {
                entityComponentStore->sharedComponents.removeReference(srcSharedComponentValues[o]);
                o++;
            }
            else // added to dstArchetype
            {
                entityComponentStore->sharedComponents.addReference(dstSharedComponentValues[n]);
                n++;
            }
        }
        for (; n < dstCount; n++) // added to dstArchetype
            entityComponentStore->sharedComponents.addReference(dstSharedComponentValues[n]);
        for (; o < srcCount; o++) // removed from dstArchetype
            entityComponentStore->sharedComponents.removeReference(srcSharedComponentValues[o]);
    }

    uint32_t count = srcChunk->entityCount;
    bool hasEmptySlots = count < srcArchetype->chunkCapacity;

    if (hasEmptySlots)
        srcArchetype->emptySlotTrackingRemoveChunk(srcChunk);

    int32_t chunkIndexInSrcArchetype = srcChunk->listIndex;

    if (likely(dstArchetype != srcArchetype))
    {
        //Change version is overriden below
        dstArchetype->addToChunkList(srcChunk, dstSharedComponentValues, 0);
        int32_t chunkIndexInDstArchetype = srcChunk->listIndex;

        // For unchanged components: Copy versions from src to dst archetype
        // For different components:
        //   - (srcArchetype->Chunks) Remove Component In-Place. ChangeVersion:No OrderVersion:No
        //   - (dstArchetype->Chunks) Add Component In-Place. ChangeVersion:Yes OrderVersion:No

        Archetype::cloneChangeVersions(srcArchetype, chunkIndexInSrcArchetype, dstArchetype, chunkIndexInDstArchetype);

        srcChunk->listIndex = chunkIndexInSrcArchetype;
        srcArchetype->removeFromChunkList(srcChunk);
        srcChunk->listIndex = chunkIndexInDstArchetype;

        srcArchetype->entityCount -= count;
        dstArchetype->entityCount += count;
        entityComponentStore->setArchetype(srcChunk, dstArchetype);

        // Bump the order versions. Even though the ORDER hasn't changed, the archetype HAS, which must be tracked.
        // Note that srcChunk is now in dstArchetype!
        // The order version for the chunk that moved must be updated.
        dstArchetype->chunks.setOrderVersion(srcChunk->listIndex, entityComponentStore->getGlobalSystemVersion());
        // The component type order version for all types in both the source and destination archetype must be incremented,
        // since entities with these types have moved. Types in both archetypes will have their version incremented twice,
        // but that's fine; the absolute value of the order version doesn't generally matter. It just needs to increase.
        entityComponentStore->incrementComponentTypeOrderVersion(srcArchetype);
        entityComponentStore->incrementComponentTypeOrderVersion(dstArchetype);
    }
    else
    {
        // This path is used when setting the shared component value for an entire chunk.
        // We don't know which value changed at this point, so just copy them all.
        for (uint32_t i = 0, sharedComponentCount = srcArchetype->numSharedComponents(); i < sharedComponentCount; ++i)
        {
            srcArchetype->chunks.setSharedComponentValue(i, chunkIndexInSrcArchetype, dstSharedComponentValues[i]);
        }
    }

    if (hasEmptySlots)
        dstArchetype->emptySlotTrackingAddChunk(srcChunk);
}