#include "ECS/JobChunk.hpp"
#include "ECS/EntityQueryManager.hpp"
#include "ECS/EntityComponentStore.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/ComponentDependencyManager.hpp"

using namespace ECS;
void JobChunkWrapperBase::schedule(ComponentDependencyManager &cdm,ThreadPool &tp){
    JobHandle dependsOn = cdm.getDependency(this->query); 
    JobParameter param;
    param.batchCount = 1;
    param.context = this;
    param.dependsOn = dependsOn;
    param.function = &JobChunkProducer::execute;
    JobHandle handle = tp.schedule(param);
    cdm.addDependency(handle,this->query); 
}
void JobChunkProducer::execute(void *j, uint32_t from, uint32_t, JobHandle){
    if(from!=0) return;
    JobChunkWrapperBase *base = reinterpret_cast<JobChunkWrapperBase*>(j);
    span<align_ptr<Archetype>> archs {base->ecs->archetypes.data(),base->ecs->archetypes.size()};
    for (uint32_t i = 0; i < archs.size(); i++){
        Archetype &arch = *archs[i];
        if(EntityQueryManager::isMatchingArchetype(arch,base->query)){
            const_span<Chunk*> chunks = arch.chunks.getChunkIndexArray();
            for(Chunk *chunk:chunks)
                base->execute(chunk);
        }
    }
}
