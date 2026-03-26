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

        inline operator uint32_t& () {
            return value;
        }
        inline operator uint32_t () const {
            return value;
        }
        inline bool isNull() {return value == 0xffffffff;}
        /// @brief MAGIC NUMBER, maximum number of chunks ECS can hold, any value equal higther that this considered to be an invalid value
        static constexpr uint32_t Maximum =  INT32_MAX;
    private:
        // MAGIC NUMBER
        uint32_t value = 0xffffffff;
    };

    struct Chunk final {
        friend class ChunkListMap;
        friend class ArchetypeChunkData;
        friend class Archetype;
        friend class ChunkStore;
        // MAGIC NUMBER. Header size. DO NOT TOUCH.
        static constexpr uint32_t MemoryOffset = 64; // (must be cache line aligned)
        // MAGIC NUMBER. Chunk size. DO NOT TOUCH.
        static constexpr uint32_t MemorySize = 16 * 1024; /// any number larger than 0xFFFF may cause overflow in offset array!
        /// @brief Maximum usable memory size
        static constexpr uint32_t BufferSize = MemorySize - MemoryOffset;
        // lower the number, the better component version-ing performs,
        // MIN: 2 MAX: 0X4096 = (16*1024/4)
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