#if !defined(CHUNK_HPP)
#define CHUNK_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    class ChunkListMap;
    class ArchetypeChunkData;
    class Archetype;

    struct ChunkIndex {
        constexpr ChunkIndex() = default;
        constexpr ChunkIndex(const uint32_t v):value{v}{}
        constexpr ChunkIndex(const ChunkIndex&) = default;
        constexpr ChunkIndex(ChunkIndex&&) = default;
        constexpr ChunkIndex& operator = (const ChunkIndex&) = default;

        inline operator uint32_t () const {
            return value;
        }
        inline bool isNull() {return value == UINT32_MAX;}
        /// @brief MAGIC NUMBER, maximum number of chunks ECS can hold, any value equal higther that this considered to be an invalid value
        static constexpr uint32_t Maximum =  INT32_MAX;
    private:
        // MAGIC NUMBER, basic structures are initialize with the default invalid value.
        uint32_t value = UINT32_MAX;
    };

    struct Chunk final {
        friend class ChunkListMap;
        friend class ArchetypeChunkData;
        friend class Archetype;
        friend class ChunkStore;
        // MAGIC NUMBER. Header size. DO NOT TOUCH.
        /// @details Considerations: must be cache line aligned
        static constexpr uint32_t MemoryOffset = 64;
        // MAGIC NUMBER. Chunk size. DO NOT TOUCH.
        /// @details Considerations: any number larger than 0xFFFF may cause overflow in offset array!
        static constexpr uint32_t MemorySize = 16 * 1024;
        /// @brief Maximum usable memory size
        static constexpr uint32_t BufferSize = MemorySize - MemoryOffset;
        // lower the number, the better component version-ing performs,
        /// @details Considerations: ArchetypeChunkData may uses 128 bit bitset for enabling bit per type for entities in a chunk
        static constexpr uint32_t MaximumEntitiesPerChunk = 128;
    protected:
        ChunkIndex index = ChunkIndex();
        Archetype *archetype = nullptr;
        uint32_t entityCount = 0;
         int32_t listWithEmptySlotsIndex = -1;
         int32_t listIndex = -1;
        alignas(MemoryOffset) uint8_t memory[BufferSize];
    };
    static_assert(sizeof(Chunk) == Chunk::MemorySize);
}

#endif