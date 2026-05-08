#if !defined(JOBCHUNK_HPP)
#define JOBCHUNK_HPP

#include "Base/IJobChunk.hpp"
#include "Base/Query.hpp"
#include "Base/Job.hpp"

namespace ECS
{
    struct Chunk;
    struct Archetype;
    struct ComponentDependencyManager;
    struct JobChunkWrapperBase {
        JobHandle schedule(EntityQueryImpl query,ComponentDependencyManager &);
        JobHandle scheduleParallel(EntityQueryImpl query,ComponentDependencyManager &);
    private:
        /// @brief JobChunkProducer
        static void execute(void *, uint32_t, uint32_t);
        virtual void execute(const Chunk*, const_span<int32_t>) = 0;
        const EntityQueryData *query = nullptr;
    };
    template<typename IJOB>
    struct JobChunkWrapper : JobChunkWrapperBase {
        static_assert(std::is_base_of_v<IJobChunk,IJOB>);
        IJOB jobData;
    private:
        void execute(const Chunk* arg, const_span<int32_t> index){
            jobData.execute(arg, index);
        }
    };
} // namespace ECS


#endif // JOBCHUNK_HPP
