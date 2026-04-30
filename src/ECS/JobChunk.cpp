#include "ECS/JobChunk.hpp"
#include "ECS/EntityQueryManager.hpp"
#include "ECS/EntityComponentStore.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/ComponentDependencyManager.hpp"

using namespace ECS;
JobHandle JobChunkWrapperBase::schedule(EntityQueryImpl _query,ComponentDependencyManager &cdm){
    this->query = _query.getData();
    JobHandle dependsOn = cdm.getDependency(*this->query); 
    JobParameter param;
    param.batchCount = 1;
    param.context = this;
    param.dependsOn = dependsOn;
    param.function = &execute;
    JobHandle handle = JobsUtility.schedule(param);
    cdm.addDependency(handle,*this->query); 
    return handle;
}
void JobChunkWrapperBase::execute(void *j, uint32_t from, uint32_t, JobHandle){
    if(from!=0) return;

    JobChunkWrapperBase *base = reinterpret_cast<JobChunkWrapperBase*>(j);
    const EntityQueryData *query = base->query;
    const int32_t                *typesIndex = query->typesIndex;
    const EntityQueryData::ChunkCache *cache = query->cache.get();
    uint32_t                      cacheCount = query->cacheCount;
    uint32_t                      typesCount = query->firstNoneIndex;

    while(cacheCount--){
        base->execute(
            cache->value,
            const_span<int32_t>{typesIndex + (typesCount * cache->archetypeIndex),typesCount}
        );
        cache++;
    }
}
