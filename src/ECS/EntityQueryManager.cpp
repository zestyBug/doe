#include "ECS/EntityQueryManager.hpp"
#include "ECS/Base/Query.hpp"
#include "ECS/Archetype.hpp"
#include "ECS/EntityComponentStore.hpp"

using namespace ECS;
bool EntityQueryManager::testMatchingArchetypeRequiredComponent(const_span<TypeID> archetypeTypes, const_span<EntityQueryData::TypeQuery> queryTypes){
    if (queryTypes.size() == 0)
        return true; // no types to search for
    uint32_t iNextQueryType = 0;
    TypeID nextQueryType = queryTypes[iNextQueryType].type;
    for (uint32_t i = 0; i < archetypeTypes.size(); i++)
    {
        TypeID componentTypeIndex = archetypeTypes[i];
        if (unlikely(componentTypeIndex == nextQueryType))
        {
            if (++iNextQueryType == queryTypes.size())
                return true; // Ran out of query types; all required types were found.
            nextQueryType = queryTypes[iNextQueryType].type;
        }
        else if (componentTypeIndex > nextQueryType)
            return false; // A required type was not found
    }
    // Ran out of archetype types & didn't find all required types
    return false;
}
bool EntityQueryManager::testMatchingArchetypeOptionalComponent(const_span<TypeID> archetypeTypes, const_span<EntityQueryData::TypeQuery> queryTypes){
    if (queryTypes.size() == 0)
        return true; // no types to search for
    uint32_t iNextQueryType = 0;
    TypeID nextQueryType = queryTypes[iNextQueryType].type;
    for (uint32_t i = 0; i < archetypeTypes.size(); i++)
    {
        TypeID componentTypeIndex = archetypeTypes[i];
        while(componentTypeIndex > nextQueryType)
        {
            if (++iNextQueryType == queryTypes.size())
                return false; // Ran out of optional types and didn't find at least one of them
            nextQueryType = queryTypes[iNextQueryType].type;
        }
        if (unlikely(componentTypeIndex == nextQueryType))
            return true; // found at least one optional type
    }
    // Ran out of archetype types & didn't find all required types
    return false;
}
bool EntityQueryManager::testMatchingArchetypeExcludedComponent(const_span<TypeID> archetypeTypes, const_span<EntityQueryData::TypeQuery> queryTypes){
    if (queryTypes.size() == 0)
        return true; // no types to search for
    uint32_t iNextQueryType = 0;
    TypeID nextQueryType = queryTypes[iNextQueryType].type;
    for (uint32_t i = 0; i < archetypeTypes.size(); i++)
    {
        TypeID componentTypeIndex = archetypeTypes[i];
        while(componentTypeIndex > nextQueryType)
        {
            if (++iNextQueryType == queryTypes.size())
                return true; // Ran out of query types. No excluded types were found
            nextQueryType = queryTypes[iNextQueryType].type;
        }
        if (unlikely(componentTypeIndex == nextQueryType))
            return false; // An excluded type was found in archetype
    }
    // Ran out of archetype types & didn't find any excluded types
    return true;
}
void EntityQueryManager::addArchetypeIfMatching(Archetype *archetype, EntityQueryData &query){
    const uint32_t noneCount = query.queryCount - query.firstNoneIndex;
    const uint32_t anyCount = query.firstNoneIndex - query.firstAnyIndex;
    const uint32_t allCount = query.firstAnyIndex;
    const_span<ECS::TypeID> archetypeTypes = archetype->getTypes();
    EntityQueryData::TypeQuery *queries = query.queries.get();
    if (archetypeTypes.size() < allCount)
        return;
    if(!testMatchingArchetypeRequiredComponent(archetypeTypes, {queries, allCount}))
        return;
    if(!testMatchingArchetypeOptionalComponent(archetypeTypes, {queries + query.firstAnyIndex , anyCount }))
        return;
    if(!testMatchingArchetypeExcludedComponent(archetypeTypes, {queries + query.firstNoneIndex, noneCount}))
        return;
    
    if(archetype->queryMask.test(query.ID))
        return;
    archetype->matchingQueryData[archetype->matchingQueryCount++] = &query;
    archetype->queryMask.set(query.ID,true);
    query.invalidateCache();

    const uint32_t archetypeIndex = query.archetypesCount;
    const uint32_t queryCount = query.firstNoneIndex;
    int32_t lastTypeIndexInTypeArray = 0;
    EntityQueryData::ArchetypeCache *ptr0;
    int32_t *ptr1;
    if(unlikely(archetypeIndex >= query.archetypesCapacity))
    {
        uint32_t size[2];
        size[0] =           (archetypeIndex + 32) * (uint32_t)sizeof(EntityQueryData::ArchetypeCache);
        size[1] = size[0] + (archetypeIndex + 32) * (uint32_t)sizeof(int32_t) * queryCount;
        {
            uint8_t * ptr = allocator().allocate(size[1]);
            ptr0 = (EntityQueryData::ArchetypeCache*)ptr;
            ptr1 = (int32_t*)(ptr + size[0]);
        }
        memcpy(ptr0, query.archetypes.get(), sizeof(EntityQueryData::ArchetypeCache) * archetypeIndex);
        memcpy(ptr1, query.typesIndex      , sizeof(int32_t)            * queryCount * archetypeIndex);
        query.archetypes.reset(ptr0);
        query.typesIndex = ptr1;
        query.archetypesCapacity += 32;
    }else{
        ptr0 = query.archetypes.get();
        ptr1 = query.typesIndex;
    }

    query.archetypesCount++;
    ptr0[archetypeIndex] = archetype;
    ptr1 += archetypeIndex * queryCount;
    for (uint32_t i = 0; i < allCount; i++)
    {
        lastTypeIndexInTypeArray = archetype->getNextIndexInTypeArray(queries[i].type,lastTypeIndexInTypeArray);
        if(lastTypeIndexInTypeArray < 0)
            throw std::runtime_error("addArchetypeIfMatching(): type unexpectedly not found");
        ptr1[queries[i].parameterIndex] = lastTypeIndexInTypeArray;
    }
    lastTypeIndexInTypeArray = 0;
    for (uint32_t i = query.firstAnyIndex; i < anyCount; i++)
    {
        int32_t currentTypeComponentIndex = archetype->getNextIndexInTypeArray(queries[i].type,lastTypeIndexInTypeArray);
        if(currentTypeComponentIndex >= 0)
            lastTypeIndexInTypeArray = currentTypeComponentIndex;
        ptr1[queries[i].parameterIndex] = currentTypeComponentIndex;
    }
}
EntityQueryImpl EntityQueryManager::createEntityQuery(const EntityQueryBuilder& query)
{
    uint32_t qcount = query.count;
    if(qcount < 1 || qcount > EntityQueryBuilder::capacity)
        throw std::invalid_argument("createEntityQuery(): invalid query");
    uint32_t id = this->entityQueryDatas.size();
    EntityQueryData *queryData = &this->entityQueryDatas.emplace_back();
    EntityQueryData::TypeQuery *queryArray;
    uint8_t *ptr;
    uint32_t size[2];

    queryData->queries = std::make_unique<EntityQueryData::TypeQuery[]>(qcount);
    queryData->queryCount = qcount;
    queryArray = queryData->queries.get();
    for(uint32_t i=0;i<qcount;++i){
        queryArray[i].type = query._all[i];
        queryArray[i].flags = query._flags[i];
        queryArray[i].parameterIndex = (uint16_t)i;
    }
    std::sort(queryArray,queryArray+qcount);
    if(queryArray[0].flags & EntityQueryData::TypeQuery::NoneFlag)
        throw std::invalid_argument("createEntityQuery(): query with no All/Any types is undefined");
    {
        do queryData->firstNoneIndex = qcount;
        while (queryArray[--qcount].flags & EntityQueryData::TypeQuery::NoneFlag);
        qcount++;
        do queryData->firstAnyIndex = qcount;
        while (queryArray[--qcount].flags & EntityQueryData::TypeQuery::AnyFlag);
    }

    size[0] =           (uint32_t)sizeof(EntityQueryData::ArchetypeCache) * Constants::InitialArchetypeCacheSize;
    size[1] = size[0] + (uint32_t)sizeof(uint32_t)                        * Constants::InitialArchetypeCacheSize * queryData->firstNoneIndex;
    ptr = std::allocator<uint8_t>().allocate(size[1]);
    queryData->archetypes.reset((EntityQueryData::ArchetypeCache*)ptr);
    queryData->typesIndex = (int32_t*)(ptr + size[0]);
    queryData->archetypesCapacity = Constants::InitialArchetypeCacheSize;
    queryData->archetypesCount = 0;

    queryData->cache = std::make_unique<EntityQueryData::ChunkCache[]>(Constants::InitialChunkCacheSize);
    queryData->cacheCapacity = Constants::InitialChunkCacheSize;
    queryData->cacheCount = 0;
    queryData->validCache = false;
    queryData->ID = id;

    span<Archetype*> archs = this->ecs->getArchetypes();
    for(Archetype *arch:archs)
        addArchetypeIfMatching(arch,*queryData);
    return EntityQueryImpl{queryData};
}
void EntityComponentStore::cleanChangeList()
{
    Archetype *archetype = chunkListChangesTracker.head;
    while(archetype != nullptr)
    {
        span<EntityQueryData*> matchingQuery = {archetype->matchingQueryData,archetype->matchingQueryCount};
        for (uint32_t queryIndex = 0; queryIndex < matchingQuery.size(); ++queryIndex)
            matchingQuery[queryIndex]->invalidateCache();
        Archetype *nextArchetype = archetype->nextChangedArchetype;
        archetype->nextChangedArchetype = nullptr;
        archetype = nextArchetype;
    }
    chunkListChangesTracker.head = nullptr;
}
void EntityQueryManager::addAdditionalArchetypes(span<Archetype*> archetypeList)
{
    for (Archetype *arch:archetypeList)
    {
        for (uint32_t g = 0; g < entityQueryDatas.size(); ++g)
        {
            addArchetypeIfMatching(arch, entityQueryDatas[g]);
        }
    }
}
void EntityQueryManager::updateNewArchetypes()
{
    span<ECS::Archetype*> archs = ecs->getArchetypes();
    uint32_t pCount = ecs->previousArchetypeCount;
    if(pCount < archs.size()){
        ecs->previousArchetypeCount = archs.size();
        archs+=pCount;
        this->addAdditionalArchetypes(archs);
    }
}
EntityQueryData* EntityQueryImpl::getData()
{
    if(!queryData)
        throw std::runtime_error("getData(): not initialized");
    if (unlikely(!queryData->isValid()))
    {
        EntityQueryManager::rebuildMatchingChunkCache(*queryData);
    }
    return queryData;
}
void EntityQueryManager::rebuildMatchingChunkCache(EntityQueryData &query){
    EntityQueryData::ChunkCache *caches;
    EntityQueryData::ArchetypeCache *archs = query.archetypes.get();
    uint32_t total_count=0;
    uint32_t archetypeCount = query.archetypesCount;
    while(archetypeCount > 0){
        archetypeCount--;
        total_count += archs[archetypeCount]->getChunks().size();
    }
    if(total_count > query.cacheCapacity){
        total_count = alignPointerSize(total_count);
        query.cacheCapacity = total_count;
        query.cache = std::make_unique<EntityQueryData::ChunkCache[]>(total_count);
    }
    caches = query.cache.get();
    total_count=0;
    archetypeCount = query.archetypesCount;
    while(archetypeCount){
        archetypeCount--;
        const Archetype *archetype = archs[archetypeCount];
        const_span<ECS::Chunk*> chunks = archetype->getChunks();
        if (archetype->entityCount > 0)
            for (const Chunk *v:chunks){
                *caches = EntityQueryData::ChunkCache{v, archetypeCount};
                total_count++;
                caches++;
            }
    }
    query.cacheCount = total_count;
    query.validCache = 1;
}