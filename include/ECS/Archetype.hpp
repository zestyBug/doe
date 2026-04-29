#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

#include <stdint.h>
#include <vector>
#include <bitset>
#include "cutil/basics.hpp"
#include "Base/TypeID.hpp"
#include "ArchetypeChunkData.hpp"
#include "ChunkListMap.hpp"
#include "Base/Constants.hpp"

class Test;

namespace ECS
{
    class EntityComponentStore;
    struct ChunkListChanges;
    struct Chunk;
    struct EntityQueryData;
    /**
     * @brief A structure holding single archetype of components.
     * in a normal case type[0] must be Entity but this class has
     * nothing to do with data types it contain. at the end its
     * just a container.
     */
    struct Archetype final
    {
    protected:
        friend struct EntityComponentStore;
        friend struct ChunkListMap;
        friend struct ArchetypeData;
        friend struct ChunkListChanges;
        friend struct JobChunkProducer;
        friend class ::Test;

        ArchetypeChunkData chunks;
        /// @brief for archetypes with zero shared components
        std::vector<Chunk*,allocator<Chunk*>> chunksWithEmptySlots;
        /// @brief for archetypes with shared components
        ChunkListMap freeChunksBySharedComponents;

        // optimal for 16 component per archetype or less
        // 'Entity' are stored as the first type
        TypeID*   _types = nullptr;
        // faster access to TypeID::realIndecies() for iteration
        uint16_t* _realIndecies = nullptr;
        /// @brief  components offsets in the chunks
        uint32_t* _offsets = nullptr;
        uint16_t* _sizeOfs = nullptr;
        TypeManager::DefaultFunction *_dDestructor = nullptr;
        TypeManager::DefaultFunction *_dConstructor = nullptr;
        uint32_t typeCount;
        // maximum number of entities that can be fit into a single chunk
        uint32_t chunkCapacity = 0;
        uint32_t entityCount = 0;

        // Order of components in the types array is always:
        // Entity, native component data, shared components, tag components

        // any index above/equal this is zero sized
        // 1 <= firstManaged<= firstTagComponent <= firstSharedComponent <= __type_count

        uint32_t firstManagedComponent =0;
        uint32_t firstTagComponent = 0;
        uint32_t firstSharedComponent = 0;
        uint32_t instanceSize = 0;
        uint32_t instanceSizeWithOverhead = 0;

        /// @brief archetype index in ECS archetype list, used for backward access.
        uint32_t archetypeIndex=0;
        /// @brief used by EntityQueryManager
        uint32_t matchingQueryCount = 0;

        EntityComponentStore *entityComponentStore;
        Archetype* nextChangedArchetype = nullptr;
        /// @brief used by EntityQueryManager
        EntityQueryData* matchingQueryData[Constants::MaximumQueryCount];
        /// @brief used by EntityQueryManager
        std::bitset<Constants::MaximumQueryCount> queryMask;

        void addToChunkListWithEmptySlots(Chunk* chunk);
        void removeFromChunkListWithEmptySlots(Chunk* chunk);
    private:
        void emptySlotTrackingRemoveChunk(Chunk* chunk);
        void emptySlotTrackingAddChunk(Chunk* chunk);
        Chunk* getExistingChunkWithEmptySlots(const SharedComponentValues sharedComponentValues);
    public:
        inline uint32_t numNonZeroSizedTypes() const {return firstTagComponent;}
        /// @brief number of non zero sized IComponentData components (not Entity)
        inline uint32_t numNativeComponentData() const {return firstManagedComponent - 1;}
        inline uint32_t numManagedComponents()   const {return firstTagComponent - firstManagedComponent;}
        inline uint32_t numTagComponents()       const {return firstSharedComponent - firstTagComponent;}
        inline uint32_t numSharedComponents()    const {return typeCount - firstSharedComponent;}
        // this fucntion is a little more expensive than empty
        inline uint32_t count() const noexcept { return entityCount; }
        inline const_span<TypeID>   getType()   const {return {this->_types,  this->typeCount};}
        inline const_span<uint32_t> getOffset() const {return {this->_offsets,this->typeCount};}
        inline const_span<uint16_t> getSize()   const {return {this->_sizeOfs,this->typeCount};}
        inline const_span<uint16_t> getIndex()  const {return {this->_realIndecies,this->typeCount};}
        Archetype& operator =(const Archetype&) = delete;
        Archetype(const Archetype&) = delete;
        // data structor stores pointer to itself
        // any move operation requires recalculation
        Archetype(Archetype&&) = delete;
        ~Archetype() = default;
        void addToChunkList(Chunk* chunk, SharedComponentValues sharedComponentIndices, uint32_t changeVersion, ChunkListChanges&);
        void removeFromChunkList(Chunk* chunk, ChunkListChanges&);

        /**
         * ChunkDataUtility
         */
    //private:

        /// @brief iterates over types array to find 
        /// @return -1 if not found
        int32_t getIndexInTypeArray(TypeID type) const;
        /// @brief when type arrays are pre-sorted, this can be used to search linearly for a match
        int32_t getNextIndexInTypeArray(TypeID type, int32_t lastTypeIndexInTypeArray) const;
        void releaseChunk(Chunk* chunk);
        void setChunkCount(Chunk* chunk, uint32_t newCount);

        void initializeComponents(Chunk* chunk, uint32_t dstIndex, uint32_t count);
        /// @brief allocating space into a chunk (updating entity count)
        /// @param chunk the chunk
        /// @param count number of entites
        /// @param outIndex index of the first allocated entity
        /// @return actual allocated entity count (if more space was not available)
        uint32_t allocateIntoChunk(Chunk* chunk, uint32_t count, uint32_t& outIndex);
        /// @brief allocate few new entities in a given chunk
        /// @param chunk the given chunk must belong to the archetype
        /// @param entities result allocated entities
        /// @return actuall allocated entity count if not enough space was available
        uint32_t allocate(Chunk* chunk, uint32_t count, Entity *entities = nullptr);
        void deallocate(Chunk *chunk);
        /// @brief deallocate and remove a batch of entities.
        /// @note may remove the chunk if chunk empties entirely.
        void deallocate(EntityBatchInChunk batch);
        /// @brief remove a batch of entities in a chunk and fill the space if required. No destructor is called.
        /// @note may remove the chunk if chunk empties entirely.
        static void remove(EntityBatchInChunk batch);
        static void copy(Chunk *srcChunk, uint32_t srcIndex, Chunk *dstChunk, uint32_t dstIndex, uint32_t count);
        static void copyComponents(Chunk *srcChunk, uint32_t srcIndex, Chunk *dstChunk, uint32_t dstIndex, uint32_t count, uint32_t dstGlobalSystemVersion);
        /// @brief Check if non-zero-sized components are same. Means chunks can be moved between archetypes.
        static bool areLayoutCompatible(Archetype *a, Archetype *b);

        /// @brief 
        /// @param srcArchetype 
        /// @param srcBatch 
        /// @param dstArchetype 
        /// @param dstChunk 
        static void clone(Archetype *srcArchetype, EntityBatchInChunk srcBatch, Archetype *dstArchetype, Chunk *dstChunk);
        /// @brief move sized components data from a Chunk to another Chunk
        /// @details operations are: copy: memcpy(dst,src), new added: memset(dst,0), removed: destruct(src)
        /// @warning assuming srcArchetype.Types[0] == dstArchetype.Types[0] == Entity
        static void convert(Archetype *srcArchetype, Chunk *srcChunk, uint32_t srcIndex, Archetype *dstArchetype, Chunk *dstChunk, uint32_t dstIndex, uint32_t count);
        static void cloneChangeVersions(Archetype* srcArchetype, int32_t chunkIndexInSrcArchetype, Archetype* dstArchetype, int32_t chunkIndexInDstArchetype, bool dstValidExistingVersions = false);
        static void changeArchetypeInPlace(Archetype* srcArchetype, Chunk *srcChunk, Archetype* dstArchetype, const SharedComponentValues sharedComponentValues);

        void addEmptyChunk(Chunk *chunk, const SharedComponentValues sharedComponentValues);

        void setSharedComponentDataIndex(Entity entity, const SharedComponentValues sharedComponentValues, TypeID typeIndex);
        void setSharedComponentDataIndex(Chunk *chunk, const SharedComponentValues sharedComponentValues, TypeID typeIndex);
        void setSharedComponentDataIndex(EntityBatchInChunk batch, const SharedComponentValues sharedComponentValues, TypeID typeIndex);
        const uint8_t* getComponentDataWithTypeRO(Chunk *chunk, uint32_t baseEntityIndex, TypeID typeIndex) const;
        const uint8_t* getComponentDataRO(Chunk *chunk, uint32_t baseEntityIndex, uint32_t indexInTypeArray) const;
        uint8_t* getComponentDataWithTypeRW(Chunk *chunk, uint32_t baseEntityIndex, TypeID typeIndex, Version globalSystemVersion);
        uint8_t* getComponentDataRW(Chunk *chunk, uint32_t baseEntityIndex, uint32_t indexInTypeArray, Version globalSystemVersion);
    };
    struct ArchetypeData
    {
        static constexpr uint32_t BufferSize = 1024;
        uint8_t buffer[BufferSize];
    };
} // namespace ECS

#endif // ARCHETYPE_HPP
