#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

#include <stdint.h>
#include <vector>
#include "Base/TypeID.hpp"
#include "ArchetypeChunkData.hpp"
#include "cutil/basics.hpp"
#include "cutil/mark_ptr.hpp"
#include "cutil/HashHelper.hpp"
#include "Base/Chunk.hpp"
#include "ChunkListMap.hpp"

class Test;

namespace ECS
{
    class EntityComponentStore;
    /**
     * @brief A structure holding single archetype of components.
     * in a normal case type[0] must be Entity but this class has
     * nothing to do with data types it contain. at the end its
     * just a container.
     */
    struct Archetype final
    {
    protected:

        friend class ChunkJobFunction;
        friend class EntityComponentManager;
        friend class ChunkListMap;
        friend struct ArchetypeData;
        friend class ::Test;

        // maximum number of entities that can be fit into a single chunk
        uint32_t chunkCapacity = 0;
        uint32_t entityCount = 0;
        uint32_t instanceSize = 0;
        uint32_t instanceSizeWithOverhead = 0;
        ArchetypeChunkData chunks;

        std::vector<Chunk*> chunksWithEmptySlots;
        ChunkListMap freeChunksBySharedComponents;

        // optimal for 16 component per archtype or less
        // 'Entity' are stored as the first type
        TypeID* __types = nullptr;
        // faster access to TypeID::realIndecies() for iteration
        uint16_t* __realIndecies = nullptr;
        /// @brief  components offsets in the chunks
        uint32_t* __offsets = nullptr;
        uint16_t* __sizeOfs = nullptr;
        uint32_t __type_count;

        // Order of components in the types array is always:
        // Entity, native component data, shared components, tag components

        // any index above/equal this is zero sized
        // 1 <= firstTagComponent <= firstSharedComponent <= __type_count

        uint32_t firstTagComponent = 0;
        uint32_t firstSharedComponent = 0;

        /// @brief archetype index in ECS archetype list, used for backward access.
        uint32_t archetypeIndex=0;

        void addToChunkListWithEmptySlots(Chunk* chunk);
        void removeFromChunkListWithEmptySlots(Chunk* chunk);
    private:
        void emptySlotTrackingRemoveChunk(Chunk* chunk);
        void emptySlotTrackingAddChunk(Chunk* chunk);
        Chunk* getExistingChunkWithEmptySlots(SharedComponentValues sharedComponentValues);
    public:
        /// @brief MAGIC NUMBER, maximum number of type an archetype can manage, any number higher than this may lead to overflow
        static constexpr uint32_t MaximumComponentCount = 0x6f;
        uint32_t numNativeComponentData() {return firstTagComponent - 1;}
        uint32_t numTagComponents() {return firstSharedComponent - firstTagComponent;}
        uint32_t numSharedComponents() {return __type_count - firstSharedComponent;}
        uint32_t numZeroSizedComponents() {return __type_count - firstTagComponent;}
        // this fucntion is a little more expensive than empty
        uint32_t count() const noexcept { return entityCount; }
        inline const_span<TypeID>   getType()   const {return {this->__types,  this->__type_count};}
        inline const_span<uint32_t> getOffset() const {return {this->__offsets,this->__type_count};}
        inline const_span<uint16_t> getSize()   const {return {this->__sizeOfs,this->__type_count};}
        inline const_span<uint16_t> getIndex()  const {return {this->__realIndecies,this->__type_count};}
        Archetype& operator =(const Archetype&) = delete;
        Archetype(const Archetype&) = delete;
        // data structor stores pointer to itself
        // any move operation requires recalculation
        Archetype(Archetype&&) = delete;
        ~Archetype() = default;
        void addToChunkList(Chunk* chunk, SharedComponentValues sharedComponentIndices, uint32_t changeVersion);
        void removeFromChunkList(Chunk* chunk);
    };
    struct ArchetypeData
    {
        static constexpr uint32_t BufferSize = 1024;
        uint8_t buffer[BufferSize];
    };
} // namespace ECS

#endif // ARCHETYPE_HPP
