#if !defined(JOBCHUNK_HPP)
#define JOBCHUNK_HPP

#include "Base/IJobChunk.hpp"
#include "Base/ArchetypeQuery.hpp"
#include "Base/Job.hpp"

namespace ECS
{
    struct EntityComponentStore;
    struct Chunk;
    struct Archetype;
    struct ThreadPool;
    struct ComponentDependencyManager;
    struct JobChunkProducer {
        static void execute(void *j, uint32_t, uint32_t, JobHandle);
    };
    struct JobChunkWrapperBase {
        virtual void execute(Chunk*) = 0;
        ArchetypeQuery query;
        EntityComponentStore *ecs;
        void schedule(ComponentDependencyManager&,ThreadPool&);
        JobChunkWrapperBase(EntityComponentStore *pecs):ecs{pecs}{}
    };
    template<typename IJOB>
    struct JobChunkWrapper : JobChunkWrapperBase {
        static_assert(std::is_base_of_v<IJobChunk,IJOB>);
        IJOB jobData;
        void execute(Chunk* arg){
            jobData.execute(arg);
        }
        JobChunkWrapper(EntityComponentStore *pecs):JobChunkWrapperBase{pecs}{}
    };
} // namespace ECS


#endif // JOBCHUNK_HPP
