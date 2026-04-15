#include "ECS/EntityComponentStore.hpp"
using namespace ECS;
EntityComponentStore::~EntityComponentStore(){
    // Move all chunks to become pooled chunks
    for (align_ptr<Archetype> &archetype: archetypes)
    {
        for (uint32_t c = 0; c != archetype->chunks.count(); c++)
        {
            Chunk* chunk = archetype->chunks[c];
            this->deallocateManagedComponents(EntityBatchInChunk{.chunk=chunk, .startIndex=0, .count = chunk->entityCount});
            this->entityStore.deallocateEntities({(Entity*)chunk->buffer, chunk->entityCount});
        }
    }
}
align_ptr<EntityComponentStore> EntityComponentStore::create(){
    align_ptr<EntityComponentStore> ret = make_align<EntityComponentStore>();
    new (&ret->typeLookup) ArchetypeListMap();
    new (&ret->archetypes) std::vector<align_ptr<Archetype>,allocator<align_ptr<Archetype>>>();
    new (&ret->entityStore) EntityStore();
    new (&ret->sharedComponents) SharedComponentStore();
    new (&ret->componentTypeOrderVersion) align_ptr<Version[]>();
    new (&ret->globalVersion) Version();
    new (&ret->chunks) ChunkStore();
    ret->componentTypeOrderVersion = make_align<Version[]>(TypeID::MaximumTypesCount);
    ret->typeLookup.init(16);
    ret->archetypes.reserve(16);
    return std::move(ret);
}
uint32_t EntityComponentStore::calculateSpaceRequirement(const_span<uint16_t> componentSizes, uint32_t entityCount)
{
    uint32_t size = 0;
    for (const auto& componentSize : componentSizes)
        size += alignTo64(componentSize, entityCount);
    return size;
}
uint32_t EntityComponentStore::calculateChunkCapacity(const_span<uint16_t> componentSizes, uint32_t bufferSize)
{
    uint32_t totalSize = 0;
    for (const auto& componentSize : componentSizes)
        totalSize += componentSize;
    uint32_t capacity = bufferSize / totalSize;
    while (calculateSpaceRequirement(componentSizes, capacity) > bufferSize)
        --capacity;
    return capacity;
}
void EntityComponentStore::validateArchetype(const_span<TypeID> types) {
    if(unlikely(types.size() < 1 || types.size() > Archetype::MaximumComponentCount))
        throw std::invalid_argument("validateArchetype(): archetype with unexpected component count");
    if(types[0] != getTypeID<Entity>())
        throw std::invalid_argument("validateArchetype(): the first type must be Entity");
}
void EntityComponentStore::incrementComponentOrderVersion(Archetype* archetype, SharedComponentValues sharedComponentValues)
{
    for (uint32_t i = 0; i < archetype->numSharedComponents(); i++)
    {
        SharedComponentIndex sharedComponentIndex = sharedComponentValues[i];
        this->sharedComponents.incrementVersion(sharedComponentIndex);
    }
}
void EntityComponentStore::incrementComponentTypeOrderVersion(const Archetype* archetype)
{
    // Increment type component version
    for (uint32_t t = 0; t < archetype->typeCount; ++t)
        componentTypeOrderVersion[archetype->_types[t].index()].updateVersion();
}
Archetype* EntityComponentStore::createArchetype(const_span<TypeID> types){
    validateArchetype(types);
    align_ptr<Archetype> arch;
    uint32_t numSharedComponents = 0;
    for (uint32_t i = 0; i < types.size(); ++i) {
        const auto& ct = TypeManager::GetTypeInfo(types[i]);
        if (ct.Category == TypeManager::TypeCategory::ISharedComponentData)
            ++numSharedComponents;
    }
    if(Archetype::MaximumSharedComponentCount < numSharedComponents)
        throw std::invalid_argument("validateArchetype(): too shareed components");
    {
        uint32_t offsets[7];
        offsets[0] =              alignTo64(sizeof(Archetype),1);
        offsets[1] = offsets[0] + alignTo64(sizeof(TypeID),types.size());
        offsets[2] = offsets[1] + alignTo64(sizeof(uint16_t),types.size());
        offsets[3] = offsets[2] + alignTo64(sizeof(uint32_t),types.size());
        offsets[4] = offsets[3] + alignTo64(sizeof(uint16_t),types.size());
        offsets[5] = offsets[4] + alignTo64(sizeof(TypeManager::DefaultFunction),types.size());
        offsets[6] = offsets[5] + alignTo64(sizeof(TypeManager::DefaultFunction),types.size());
        arch.reset((Archetype*)allocator().allocate(offsets[6]));
        new (&arch->chunks) ArchetypeChunkData(types.size(),numSharedComponents);
        new (&arch->chunksWithEmptySlots) std::vector<Chunk*,allocator<Chunk*>>();
        arch->chunksWithEmptySlots.reserve(16);
        new (&arch->freeChunksBySharedComponents) ChunkListMap();
        arch->freeChunksBySharedComponents.init(arch.get());
        arch->_types        = (TypeID*)  ((uint8_t*)(arch.get()) + offsets[0]);
        arch->_realIndecies = (uint16_t*)((uint8_t*)(arch.get()) + offsets[1]);
        arch->_offsets      = (uint32_t*)((uint8_t*)(arch.get()) + offsets[2]);
        arch->_sizeOfs      = (uint16_t*)((uint8_t*)(arch.get()) + offsets[3]);
        arch->_dDestructor  = (TypeManager::DefaultFunction*)((uint8_t*)(arch.get()) + offsets[4]);
        arch->_dConstructor = (TypeManager::DefaultFunction*)((uint8_t*)(arch.get()) + offsets[5]);
    }
    arch->typeCount   = types.size();
    arch->chunkCapacity = 0;
    arch->entityCount = 0;
    {
        uint16_t i = (uint16_t) types.size();
        do arch->firstSharedComponent = i;
        while (types[--i].isSharedComponent());
        i++;
        do arch->firstTagComponent = i;
        while (types[--i].isZeroSized());
        i++;
        do arch->firstManagedComponent = i;
        while (types[--i].isManagedComponent());
    }
    arch->instanceSize = 0;
    arch->instanceSizeWithOverhead = 0;
    arch->entityComponentStore = this;

    memcpy(arch->_types,types.data(),types.size_bytes());
    for (uint32_t i = 0; i < types.size(); ++i)
        arch->_realIndecies[i] = (uint16_t)types[i].index();
    for (uint32_t i = 0; i < types.size(); ++i)
        arch->_sizeOfs[i] = (uint16_t) TypeManager::GetTypeInfo(types[i]).SizeInChunk;
    for (uint32_t i = 0; i < types.size(); ++i)
        arch->_dDestructor[i] = TypeManager::GetTypeInfo(types[i]).defaultDestruct;
    for (uint32_t i = 0; i < types.size(); ++i)
        arch->_dConstructor[i] = TypeManager::GetTypeInfo(types[i]).defaultConstruct;


    arch->chunkCapacity = std::min(
        calculateChunkCapacity({arch->_sizeOfs,types.size()},Chunk::BufferSize),
        Chunk::MaximumEntitiesPerChunk
    );
    for (uint32_t i = 0,usedBytes = Chunk::MemoryOffset; i < types.size(); i++)
    {
        arch->_offsets[i] = usedBytes;
        usedBytes += alignTo64(arch->_sizeOfs[i], arch->chunkCapacity);
    }
    for (uint32_t i = 0; i < arch->numNonZeroSizedTypes(); i++)
    {
        arch->instanceSize += arch->_sizeOfs[i];
        arch->instanceSizeWithOverhead += alignTo64(arch->_sizeOfs[i], 1);
    }
    this->archetypes.emplace_back(arch.get());
    this->typeLookup.add(arch.get());
    return arch.release();
}
Archetype* EntityComponentStore::getOrCreateArchetype(const_span<TypeID> types){
    Archetype* arch = getExistingArchetype(types);
    if (arch != nullptr)
        return arch;
    return createArchetype(types);
}
void EntityComponentStore::moveAndSetChangeVersion(
    EntityBatchInChunk batch, 
    Archetype *archetype, 
    const SharedComponentValues sharedComponentValues, 
    TypeID type)
{
    if ((batch.count == batch.chunk->entityCount))
    {
        if(batch.startIndex != 0)
            throw std::invalid_argument("moveAndSetChangeVersion(): invalid batch");
        Archetype *srcArchetype = getArchetype(batch.chunk);
        srcArchetype->deallocate(batch);
        return;
    }

    int32_t typeIndexInDstArchetype = archetype->getIndexInTypeArray(type);
    if(typeIndexInDstArchetype < 0)
        throw std::invalid_argument("moveAndSetChangeVersion(): type not found in archetype");
    while (batch.count > 0)
    {
        Chunk *dstChunk = getChunkWithEmptySlots(archetype, sharedComponentValues);
        uint32_t dstCount = move(batch, dstChunk);
        batch.count -= dstCount;
        archetype->chunks.setChangeVersion(typeIndexInDstArchetype, dstChunk->listIndex, getGlobalSystemVersion());
    }
}




Archetype* EntityComponentStore::getArchetype(Entity entity){
    validateEntities({&entity, 1});
    Chunk* chunk = getChunk(entity);
    if(chunk == nullptr)
        throw std::runtime_error("getArchetype(): unexpected chunk index");
    return chunk->archetype;
}
Archetype* EntityComponentStore::getArchetype(ChunkIndex chunk){
    Chunk* pointer = chunks.getChunkPointer(chunk);
    if(!pointer || !pointer->archetype)
        throw std::runtime_error("getArchetype(): invalid chunk");
    return pointer->archetype;
}
Archetype* EntityComponentStore::getArchetype(Chunk* chunk){
    if(!chunk || !chunk->archetype)
        throw std::runtime_error("getArchetype(): invalid chunk");
    return chunk->archetype;
}
void EntityComponentStore::destroyBatch(EntityBatchInChunk batch){
    Archetype *arch = this->getArchetype(batch.chunk);
    arch->deallocate(batch);
}
void EntityComponentStore::freeEntities(Chunk* chunk)
{   
    this->entityStore.deallocateEntities({(Entity*)chunk->buffer, chunk->entityCount});
}
Chunk* EntityComponentStore::getChunkWithEmptySlots(Archetype* archetype, const SharedComponentValues sharedComponentIndecies)
{
    Chunk *chunk = archetype->getExistingChunkWithEmptySlots(sharedComponentIndecies);
    if (chunk == nullptr)
        chunk = this->getCleanChunk(archetype, sharedComponentIndecies);
    return chunk;
}
Chunk* EntityComponentStore::getCleanChunk(Archetype* archetype, SharedComponentValues sharedComponentValues)
{
    Chunk *newChunk = allocateChunk();
    archetype->addEmptyChunk(newChunk, sharedComponentValues);
    return newChunk;
}
Chunk* EntityComponentStore::allocateChunk()
{
    ECS::ChunkIndex newChunkIndex = chunks.allocateChunk();
    return chunks.getChunkPointer(newChunkIndex);
}
SharedComponentIndex EntityComponentStore::getSharedComponentDataIndex(Entity entity, TypeID type)
{
    if(!type.isSharedComponent())
        throw std::invalid_argument("getSharedComponentDataIndex(): invalid type");
    Archetype *archetype = this->getArchetype(entity);
    int32_t indexInTypeArray = archetype->getIndexInTypeArray(type);
    if(indexInTypeArray < 0)
        throw std::invalid_argument("getSharedComponentDataIndex(): type not found");
    Chunk *chunk = getChunk(entity);
    SharedComponentValues sharedComponentValueArray = archetype->chunks.getSharedComponentValues(chunk->listIndex);
    uint32_t sharedComponentOffset = (uint32_t)indexInTypeArray - archetype->firstSharedComponent;
    return sharedComponentValueArray[sharedComponentOffset];
}
const void* EntityComponentStore::getComponentDataWithTypeRO(Entity entity, TypeID type)
{
    EntityInChunk entityInChunk = this->getEntityInChunk(entity);
    Archetype* archetype = this->getArchetype(entityInChunk.chunk);
    return archetype->getComponentDataWithTypeRO(entityInChunk.chunk, entityInChunk.indexInChunk, type);
}
void* EntityComponentStore::getComponentDataWithTypeRW(Entity entity, TypeID type)
{
    EntityInChunk entityInChunk = this->getEntityInChunk(entity);
    Archetype *archetype = this->getArchetype(entityInChunk.chunk);
    return archetype->getComponentDataWithTypeRW(entityInChunk.chunk, entityInChunk.indexInChunk, type, globalVersion);
}

void EntityComponentStore::validateEntities(span<Entity> entities){
    for(auto entity:entities){
        if(!entity.isValid())
            throw std::out_of_range("validateEntities(): invalid index");
        if(this->entityStore.getChunkIfExists(entity) != nullptr)
            throw std::out_of_range("validateEntities(): invalid index");
    }
}
uint32_t EntityComponentStore::countEntities(){
    uint32_t total = 0;
    for (size_t i = 0; i < archetypes.size(); i++)
    {
        total += archetypes[i].get()->entityCount;
    }
    return total;
}
void EntityComponentStore::createEntities(Archetype* archetype, span<Entity> entities, SharedComponentValues values){
    while (entities.size())
    {
        Chunk* chunk = getChunkWithEmptySlots(archetype, values);
        uint32_t unusedCount = archetype->chunkCapacity - chunk->entityCount;
        uint32_t allocateCount = std::min(entities.size(), unusedCount);
        archetype->allocate(chunk, allocateCount, entities.data());
        entities += allocateCount;
    }
}
EntityBatchInChunk EntityComponentStore::getFirstEntityBatchInChunk(const_span<Entity> entities){
    EntityBatchInChunk ret;
    EntityInChunk entityInChunk;
    Entity entity;
    
    if(entities.empty())
        throw std::invalid_argument("getFirstEntityBatchInChunk(): empty array");

    entity = entities[0];
    entityInChunk = exists(entity) ? getEntityInChunk(entity) : EntityInChunk();
    ret = {
        entityInChunk.chunk,
        entityInChunk.indexInChunk,
        1
    };

    for (; ret.count < entities.size(); ret.count++)
    {
        entity = entities[ret.count];
        entityInChunk = exists(entity) ? getEntityInChunk(entity) : EntityInChunk();
        if (entityInChunk.chunk != ret.chunk || 
            entityInChunk.indexInChunk != (ret.startIndex + ret.count))
            break;
    }

    if(ret.chunk != nullptr && (ret.startIndex + ret.count) > ret.chunk->entityCount)
        throw std::runtime_error("getFirstEntityBatchInChunk():");

    return ret;
}
void EntityComponentStore::destroyEntities(const_span<Entity> entities){
    while(!entities.empty())
    {
        EntityBatchInChunk batch = getFirstEntityBatchInChunk(entities);
        if (batch.chunk == nullptr)
        {
            entities += batch.count;
            continue;
        }
        destroyBatch(batch);
        entities += batch.count;
    }
}
void EntityComponentStore::allocateEntities(Archetype* arch, Chunk *chunk, uint32_t baseIndex, uint count, Entity* outputEntities)
{
    if(arch->_types[0] != 1 || arch->_offsets[0] != 64)
        throw std::invalid_argument("allocateEntities(): invalid archetype");

    Entity* entityInChunkStart = (Entity*)(chunk->buffer) + baseIndex;

    this->entityStore.allocateEntities({entityInChunkStart, count}, chunk, baseIndex);

    if (outputEntities != nullptr)
        memcpy(outputEntities, entityInChunkStart, count * sizeof(Entity));
}
void EntityComponentStore::deallocateDataEntitiesInChunk(EntityBatchInChunk batch)
{
    if(batch.chunk->entityCount < (batch.startIndex + batch.count))
        throw std::out_of_range("deallocateDataEntitiesInChunk(): invalid batch");
    deallocateManagedComponents(batch);

    Entity* entities = (Entity*)batch.chunk->buffer + batch.startIndex;


    this->entityStore.deallocateEntities({entities, batch.count});

    // Compute the number of things that need to moved and patched.
    uint32_t patchCount = std::min(batch.count, batch.chunk->entityCount - batch.startIndex - batch.count);

    if (0 == patchCount)
        return;

    // updates indexInChunk to point to where the components will be moved to
    //Assert.IsTrue(chunk->archetype->sizeOfs[0] == sizeof(Entity) && chunk->archetype->offsets[0] == 0);
    Entity* movedEntities = (Entity*)batch.chunk->buffer + (batch.chunk->entityCount - patchCount);
    for (uint32_t i = 0; i != patchCount; i++)
    {
        EntityInChunk entityInChunk = getEntityInChunk(movedEntities[i]);
        entityInChunk.indexInChunk = batch.startIndex + i;
        this->entityStore.setEntityInChunk(movedEntities[i], entityInChunk);
    }

    // Move component data from the end to where we deleted components
    uint32_t startIndex = batch.chunk->entityCount - patchCount;

    Archetype::copy(batch.chunk, startIndex, batch.chunk, batch.startIndex, patchCount);
}
void EntityComponentStore::deallocateManagedComponents(EntityBatchInChunk batch)
{
    Archetype *archetype = getArchetype(batch.chunk);
    if (archetype->numManagedComponents() == 0)
        return;
    uint32_t firstManagedComponent = archetype->firstManagedComponent;
    uint32_t endManagedComponents = firstManagedComponent + archetype->numManagedComponents();
    for (uint32_t localTypeIndex = firstManagedComponent; localTypeIndex < endManagedComponents; ++localTypeIndex)
    {
        TypeManager::DefaultFunction dFunc = TypeManager::GetTypeInfo(archetype->_types[localTypeIndex]).defaultDestruct;
        uint32_t sizeOf = archetype->_sizeOfs[localTypeIndex];
        uint8_t *ptr = (uint8_t*)archetype->getComponentDataRO(batch.chunk, 0, localTypeIndex);
        for (uint32_t ei = 0; ei < batch.count; ++ei)
        {
            dFunc(ptr + sizeOf * (ei + batch.startIndex));
        }
    }
}
void EntityComponentStore::addExistingEntitiesInChunk(Chunk *chunk)
{
    Entity* entities = (Entity*)chunk->buffer;
    for(uint32_t iEntity = 0, count = chunk->entityCount; iEntity < count; ++iEntity)
    {
        Entity entity = entities[iEntity];
        entityStore.setEntityInChunk(entity, {chunk, iEntity});
        entityStore.setEntityVersion(entity, entity.version());
    }
}
void EntityComponentStore::setSharedComponentDataIndexForChunk(Chunk* chunk, Archetype* chunkArchetype, TypeID type, SharedComponentIndex value)
{
    SharedComponentIndex chunkFilter[Archetype::MaximumSharedComponentCount];
    // this chunk already has the desired shared component value
    if (!getArchetypeChunkFilterWithChangedSharedComponent(chunk, type, value, chunkFilter))
        return;
    const SharedComponentValues values{ chunkFilter, sizeof(SharedComponentIndex)};
    // All entities in the chunk are enabled; set the value en masse
    chunkArchetype->setSharedComponentDataIndex(chunk, values, type);
}
bool EntityComponentStore::getArchetypeChunkFilterWithChangedSharedComponent(Chunk *chunk, TypeID type, SharedComponentIndex value, SharedComponentIndex *result)
{
    if(!type.isSharedComponent())
        throw std::invalid_argument("getArchetypeChunkFilterWithChangedSharedComponent(): invalid type");
    Archetype *archetype = getArchetype(chunk);
    int32_t indexInTypeArray = archetype->getIndexInTypeArray(type);
    if(indexInTypeArray < 0)
        throw std::invalid_argument("getArchetypeChunkFilterWithChangedSharedComponent(): type not fount");

    SharedComponentValues srcSharedComponentValueArray = archetype->chunks.getSharedComponentValues(chunk->listIndex);
    uint32_t sharedComponentOffset = indexInTypeArray - archetype->firstSharedComponent;
    SharedComponentIndex srcSharedComponentValue = srcSharedComponentValueArray[sharedComponentOffset];

    if (value == srcSharedComponentValue)
        return false;

    for (uint32_t i = 0, count = archetype->numSharedComponents(); i < count; ++i)
        result[i] = srcSharedComponentValueArray[i];
    result[sharedComponentOffset] = value;
    return true;
}







bool EntityComponentStore::exists(Entity entity){
    if(!entity.isValid())
        return false;
    Chunk* chunk = this->entityStore.getChunkIfExists(entity);
    if(chunk == nullptr)
        return false;
    return true;
}
bool EntityComponentStore::hasComponent(Entity entity, TypeID type){
    bool entityExists = exists(entity);
    if (unlikely(!entityExists))
        return false;
    Archetype *arch = getArchetype(entity);
    return arch->getIndexInTypeArray(type) != -1;
}








Archetype* EntityComponentStore::getArchetypeWithAddedComponents(Archetype* srcArchetype, const_span<TypeID> types)
{
    if(0 == types.size())
        return nullptr;
    TypeID* srcTypes = srcArchetype->_types;
    uint32_t dstTypesCount = srcArchetype->typeCount + types.size();
    TypeID dstTypes[dstTypesCount];
    // zipper the two sorted arrays "type" and "componentTypeInArchetype" into "componentTypeInArchetype"
    // because this is done in-place, it must be done backwards so as not to disturb the existing contents.

    uint32_t mixedThings = dstTypesCount;
    {
        int32_t oldThings = (int32_t)srcArchetype->typeCount - 1;
        int32_t newThings = (int32_t)types.size() - 1;
        while (newThings >= 0) // oldThings[0] has typeIndex 0, newThings can't have anything lower than that
        {
            TypeID oldThing = srcTypes[oldThings];
            TypeID newThing = types[newThings];
            if (oldThing > newThing) // put whichever is bigger at the end of the array
            {
                dstTypes[--mixedThings] = oldThing;
                --oldThings;
            }
            else
            {
                if (oldThing == newThing)
                    --oldThings;
                dstTypes[--mixedThings] = newThing;
                --newThings;
            }
        }
        while (oldThings >= 0) // if there are remaining old things, copy them here
        {
            dstTypes[--mixedThings] = srcTypes[oldThings--];
        }
        /// In case we ignored duplicated types, 'mixedThings' will be > 0
    }
    if(mixedThings == types.size())
        return nullptr;
    return getOrCreateArchetype({dstTypes + mixedThings, dstTypesCount - mixedThings});
}

Archetype* EntityComponentStore::getArchetypeWithAddedComponent(Archetype* archetype, TypeID type, uint32_t* indexInTypeArray)
{
    TypeID *types = archetype->_types;
    const uint32_t oldSize = archetype->typeCount;
    TypeID newTypes[oldSize + 1];
    uint32_t t = 0;
    while (t < oldSize && types[t] < type)
    {
        newTypes[t] = types[t];
        ++t;
    }
    if (indexInTypeArray != nullptr)
        *indexInTypeArray = t;
    if (t != oldSize && types[t] == type)
        // Tag component type is already there, no new archetype required.
        return nullptr;
    newTypes[t] = type;
    while (t < oldSize)
    {
        newTypes[t + 1] = types[t];
        ++t;
    }
    return getOrCreateArchetype({newTypes, oldSize + 1});
}
Archetype* EntityComponentStore::getArchetypeWithRemovedComponent(Archetype* archetype, TypeID type, uint32_t* indexInOldTypeArray)
{
    TypeID *types = archetype->_types;
    const uint32_t oldSize = archetype->typeCount;
    TypeID newTypes[oldSize];
    uint32_t removedTypes = 0;
    for (uint32_t t = 0; t < oldSize; ++t)
        if (types[t] == type)
        {
            if (indexInOldTypeArray != nullptr)
                *indexInOldTypeArray = t;
            ++removedTypes;
        }
        else
            newTypes[t - removedTypes] = types[t];
    if(removedTypes == 0)
        return nullptr;
    return getOrCreateArchetype({newTypes, oldSize - removedTypes});
}

Archetype* EntityComponentStore::getArchetypeWithRemovedComponents(Archetype* archetype, const_span<TypeID> types)
{
    TypeID *srcTypes = archetype->_types;
    const uint32_t oldSize = archetype->typeCount;
    TypeID newTypes[oldSize];
    uint32_t numRemovedTypes = 0;
    for (uint32_t t = 0; t < oldSize; ++t)
    {
        const TypeID existingTypeIndex = srcTypes[t];
        for (TypeID type:types)
        {
            if (existingTypeIndex == type)
            {
                numRemovedTypes++;
                goto remove;
            }
        }
        newTypes[t - numRemovedTypes] = types[t];
        remove:;
    }
    if (numRemovedTypes == 0)
        return nullptr;
    return getOrCreateArchetype({newTypes, oldSize - numRemovedTypes});
}

uint32_t EntityComponentStore::move(EntityBatchInChunk srcBatch, Chunk* dstChunk)
{
    Archetype *srcArchetype = this->getArchetype(srcBatch.chunk);
    Archetype *dstArchetype = this->getArchetype(dstChunk);
    uint32_t   dstUnusedCount = dstArchetype->chunkCapacity - dstChunk->entityCount;

    EntityBatchInChunk partialSrcBatch;
    partialSrcBatch.chunk = srcBatch.chunk;
    partialSrcBatch.count = std::min(dstUnusedCount, srcBatch.count);
    partialSrcBatch.startIndex = srcBatch.startIndex + srcBatch.count - partialSrcBatch.count;

    Archetype::clone(srcArchetype, partialSrcBatch, dstArchetype, dstChunk);

    Archetype::remove(partialSrcBatch);

    return partialSrcBatch.count;
}
void EntityComponentStore::move(Entity entity, Archetype* archetype, SharedComponentValues sharedComponentValues)
{
    EntityInChunk srcEntityInChunk = this->getEntityInChunk(entity);
    this->move({srcEntityInChunk.chunk , srcEntityInChunk.indexInChunk, 1}, archetype, sharedComponentValues);
}
void EntityComponentStore::move(Chunk *chunk, Archetype* archetype, SharedComponentValues sharedComponentValues)
{
    Archetype *srcArchetype = this->getArchetype(chunk);

    if (Archetype::areLayoutCompatible(srcArchetype, archetype))
    {
        Archetype::changeArchetypeInPlace(srcArchetype, chunk, archetype, sharedComponentValues);
        return;
    }
    this->move({chunk,0,chunk->entityCount}, archetype, sharedComponentValues);
}
void EntityComponentStore::move(EntityBatchInChunk batch, Archetype* archetype, SharedComponentValues sharedComponentValues)
{
    while (batch.count > 0)
    {
        Chunk *dstChunk = this->getChunkWithEmptySlots(archetype, sharedComponentValues);
        const uint32_t moved = this->move({batch.chunk, batch.startIndex, batch.count}, dstChunk);
        batch.count -= moved;
    }
}
bool EntityComponentStore::addComponent(Entity entity, TypeID type){
    EntityInChunk eich = this->getEntityInChunk(entity);
    return this->addComponent(
        EntityBatchInChunk{.chunk = eich.chunk, .startIndex = eich.indexInChunk, .count = 1 },
        type, type.isSharedComponent() ? sharedComponents.getDefaultValue(type) : SharedComponentIndex());
}
/// @param types sorted
bool EntityComponentStore::addComponents(Entity entity, const_span<TypeID> types){
    EntityInChunk eich = this->getEntityInChunk(entity);
    return this->addComponents(EntityBatchInChunk{.chunk = eich.chunk, .startIndex = eich.indexInChunk, .count = 1 },types);
}
bool EntityComponentStore::removeComponent(Entity entity, TypeID type){
    EntityInChunk eich = this->getEntityInChunk(entity);
    return this->removeComponent(EntityBatchInChunk{.chunk = eich.chunk, .startIndex = eich.indexInChunk, .count = 1 },type);
}
/// @param types sorted
bool EntityComponentStore::removeComponents(Entity entity, const_span<TypeID> types){
    EntityInChunk eich = this->getEntityInChunk(entity);
    return this->removeComponents(EntityBatchInChunk{.chunk = eich.chunk, .startIndex = eich.indexInChunk, .count = 1 },types);
}
/// @param value shared component index, ignored if type is not a shared component.
bool EntityComponentStore::addComponent(EntityBatchInChunk entityBatchInChunk, TypeID type, SharedComponentIndex value){
    SharedComponentIndex outSharedComponentValues[Archetype::MaximumSharedComponentCount];
    uint32_t indexInTypeArray;
    Archetype *srcArchetype = getArchetype(entityBatchInChunk.chunk);
    Archetype *dstArchetype = getArchetypeWithAddedComponent(srcArchetype, type, &indexInTypeArray);
    if (dstArchetype == nullptr)
        return false;
    buildSharedComponentIndicesWithAddedComponent(entityBatchInChunk.chunk,dstArchetype,indexInTypeArray, value,outSharedComponentValues);
    this->move(entityBatchInChunk, dstArchetype, {outSharedComponentValues,sizeof(SharedComponentIndex)});
    return true;
}
bool EntityComponentStore::removeComponent(EntityBatchInChunk entityBatchInChunk, TypeID type){
    SharedComponentIndex outSharedComponentValues[Archetype::MaximumSharedComponentCount];
    uint32_t indexInTypeArray;
    Archetype *srcArchetype = getArchetype(entityBatchInChunk.chunk);
    Archetype *dstArchetype = getArchetypeWithRemovedComponent(srcArchetype, type, &indexInTypeArray);
    if (dstArchetype == nullptr)
        return false;
    buildSharedComponentIndicesWithRemovedComponent(entityBatchInChunk.chunk,dstArchetype,indexInTypeArray,outSharedComponentValues);
    this->move(entityBatchInChunk, dstArchetype, {outSharedComponentValues,sizeof(SharedComponentIndex)});
    return true;
}
/// @param types sorted
bool EntityComponentStore::addComponents(EntityBatchInChunk entityBatchInChunk, const_span<TypeID> types){
    SharedComponentIndex outSharedComponentValues[Archetype::MaximumSharedComponentCount];
    Archetype *srcArchetype = getArchetype(entityBatchInChunk.chunk);
    Archetype *dstArchetype = getArchetypeWithAddedComponents(srcArchetype, types);
    if (dstArchetype == nullptr)
        return false;
    buildSharedComponentIndicesWithAddedComponents(entityBatchInChunk.chunk,dstArchetype,outSharedComponentValues);
    this->move(entityBatchInChunk, dstArchetype, {outSharedComponentValues,sizeof(SharedComponentIndex)});
    return true;
}
/// @param types sorted
bool EntityComponentStore::removeComponents(EntityBatchInChunk entityBatchInChunk, const_span<TypeID> types){
    SharedComponentIndex outSharedComponentValues[Archetype::MaximumSharedComponentCount];
    Archetype *srcArchetype = getArchetype(entityBatchInChunk.chunk);
    Archetype *dstArchetype = getArchetypeWithRemovedComponents(srcArchetype, types);
    if (dstArchetype == nullptr)
        return false;
    buildSharedComponentIndicesWithRemovedComponents(entityBatchInChunk.chunk,dstArchetype,outSharedComponentValues);
    this->move(entityBatchInChunk, dstArchetype, {outSharedComponentValues,sizeof(SharedComponentIndex)});
    return true;
}
void EntityComponentStore::buildSharedComponentIndicesWithAddedComponents(
    Chunk* srcChunk, const Archetype* dstArchetype, SharedComponentIndex* outSharedComponentValues)
{
    const Archetype* srcArchetype = this->getArchetype(srcChunk);
    uint32_t numSrcSharedComponents = srcArchetype->numSharedComponents();
    const SharedComponentValues srcSharedComponentValues = srcArchetype->chunks.getSharedComponentValues(srcChunk->listIndex);
    
    int32_t oldFirstShared = (int32_t)srcArchetype->firstSharedComponent;
    int32_t newFirstShared = (int32_t)dstArchetype->firstSharedComponent;
    int32_t oldCount = (int32_t)srcArchetype->numSharedComponents();
    int32_t newCount = (int32_t)dstArchetype->numSharedComponents();

    if (newCount != /* > */ oldCount)
        for (int32_t oldIndex = oldCount - 1, newIndex = newCount - 1; newIndex >= 0; --newIndex)
        {
            const TypeID oldType = dstArchetype->_types[newIndex + newFirstShared];
            // oldIndex might become -1 which is ok since oldFirstShared is always at least 1. The comparison will then always be false
            if (oldType == srcArchetype->_types[oldIndex + oldFirstShared])
                outSharedComponentValues[newIndex] = srcSharedComponentValues[oldIndex--];
            else
                outSharedComponentValues[newIndex] = this->sharedComponents.getDefaultValue(oldType);
        }
    else
        for (uint32_t i = 0; i < numSrcSharedComponents; i++)
            outSharedComponentValues[i] = srcSharedComponentValues[i];
}
void EntityComponentStore::buildSharedComponentIndicesWithAddedComponent(
    Chunk* srcChunk, const Archetype* dstArchetype,
    uint32_t newTypeIndex, SharedComponentIndex value,
    SharedComponentIndex* outSharedComponentValues)
{
    const Archetype* srcArchetype = this->getArchetype(srcChunk);
    const SharedComponentValues oldSharedComponentValues = srcArchetype->chunks.getSharedComponentValues(srcChunk->listIndex);
    uint32_t newFirstShared = dstArchetype->firstSharedComponent;
    uint32_t oldCount       = srcArchetype->numSharedComponents();
    uint32_t newCount       = dstArchetype->numSharedComponents();
    uint32_t indexOfNewSharedComponent = newTypeIndex - newFirstShared;
    if(oldCount != newCount){
        if(newTypeIndex < newFirstShared || indexOfNewSharedComponent >= newCount)
            throw std::runtime_error("buildSharedComponentIndicesWithAddedComponent(): internal error");
        if((oldCount+1) != newCount)
            throw std::invalid_argument("buildSharedComponentIndicesWithAddedComponent(): archetypes dont match the function");
        for (uint32_t i = 0; i < newCount; i++)
        {
            if(i == indexOfNewSharedComponent)
            {
                outSharedComponentValues[indexOfNewSharedComponent] = value;
                outSharedComponentValues++;
                continue;
            }
            outSharedComponentValues[i] = oldSharedComponentValues[i];
        }
    } else
        for(uint32_t i = 0; i < oldCount; i++)
            outSharedComponentValues[i] = oldSharedComponentValues[i];
}
void EntityComponentStore::buildSharedComponentIndicesWithRemovedComponents(
    Chunk* srcChunk, const Archetype* dstArchetype,
    SharedComponentIndex* outSharedComponentValues)
{
    const Archetype* srcArchetype = this->getArchetype(srcChunk);
    uint32_t numSrcSharedComponents = srcArchetype->numSharedComponents();
    const SharedComponentValues srcSharedComponentValues = srcArchetype->chunks.getSharedComponentValues(srcChunk->listIndex);

    uint32_t oldFirstShared = srcArchetype->firstSharedComponent;
    uint32_t newFirstShared = dstArchetype->firstSharedComponent;
    uint32_t newCount = dstArchetype->numSharedComponents();
    uint32_t oldCount = srcArchetype->numSharedComponents();

    if (newCount != /* < */ oldCount)
        for (uint32_t i = 0; i < newCount; i++)
        {
            // find index of srcType that matches dstType
            TypeID oldType = dstArchetype->_types[oldFirstShared + i];
            uint32_t matchingSrcIdx = 0;
            for (uint32_t j = 0; j < oldCount; j++)
            {
                TypeID newType = srcArchetype->_types[newFirstShared + j];
                if (newType == oldType)
                {
                    matchingSrcIdx = j;
                    break;
                }
            }

            outSharedComponentValues[i] = srcSharedComponentValues[(int32_t)matchingSrcIdx];
        }
    else
        for (uint32_t i = 0; i < numSrcSharedComponents; i++)
            outSharedComponentValues[i] = srcSharedComponentValues[i];
}
void EntityComponentStore::buildSharedComponentIndicesWithRemovedComponent(
    Chunk* srcChunk, const Archetype* dstArchetype,
    uint32_t oldTypeIndex, SharedComponentIndex* outSharedComponentValues)
{
    const Archetype* srcArchetype = this->getArchetype(srcChunk);
    const SharedComponentValues oldSharedComponentValues = srcArchetype->chunks.getSharedComponentValues(srcChunk->listIndex);
    uint32_t oldFirstShared = srcArchetype->firstSharedComponent;
    uint32_t oldCount       = srcArchetype->numSharedComponents();
    uint32_t newCount       = dstArchetype->numSharedComponents();
    uint32_t indexOfRemovedSharedComponent = oldTypeIndex - oldFirstShared;

    if(oldCount != newCount){
        if(oldTypeIndex < oldFirstShared || indexOfRemovedSharedComponent >= oldCount)
            throw std::runtime_error("buildSharedComponentIndicesWithRemovedComponent(): internal error");
        if(oldCount != (newCount+1))
            throw std::invalid_argument("buildSharedComponentIndicesWithRemovedComponent(): archetypes dont match the function");
        for (uint32_t i = 0; i < newCount; i++)
        {
            if(i == indexOfRemovedSharedComponent){
                outSharedComponentValues--;
                continue;
            }
            outSharedComponentValues[i] = oldSharedComponentValues[i];
        }
    }else{
        for(uint32_t i = 0; i < oldCount; i++)
            outSharedComponentValues[i] = oldSharedComponentValues[i];
    }
}