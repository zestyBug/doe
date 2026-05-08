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
    param.batchStepSize = this->query->cacheCount;
    param.context = this;
    param.dependsOn = dependsOn;
    param.function = &execute;
    JobHandle handle = JobsUtility::schedule(param);
    cdm.addDependency(handle,*this->query); 
    return handle;
}
JobHandle JobChunkWrapperBase::scheduleParallel(EntityQueryImpl _query,ComponentDependencyManager &cdm){
    this->query = _query.getData();
    JobHandle dependsOn = cdm.getDependency(*this->query); 
    JobParameter param;
    param.batchCount = this->query->cacheCount;
    param.batchStepSize = 1;
    param.context = this;
    param.dependsOn = dependsOn;
    param.function = &execute;
    JobHandle handle = JobsUtility::schedule(param);
    cdm.addDependency(handle,*this->query); 
    return handle;
}
void JobChunkWrapperBase::execute(void *j, uint32_t from, uint32_t to){
    JobChunkWrapperBase          *base = reinterpret_cast<JobChunkWrapperBase*>(j);
    const EntityQueryData        *query = base->query;
    const int32_t                *typesIndex = query->typesIndex;
    const uint32_t                cacheCount = query->cacheCount;
    const uint32_t                typesCount = query->firstNoneIndex;
    const EntityQueryData::ChunkCache *cacheFrom = query->cache.get() + from;
    const EntityQueryData::ChunkCache *cacheTo = query->cache.get() + std::min<uint32_t>(to,cacheCount);
    while(cacheFrom < cacheTo){
        base->execute(
            cacheFrom->value,
            const_span<int32_t>{typesIndex + (typesCount * cacheFrom->archetypeIndex),typesCount}
        );
        cacheFrom++;
    }
}
