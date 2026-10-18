#if !defined(ChunkStore_HPP)
#define ChunkStore_HPP

#include <array>
#include <atomic>
#include "Base/Chunk.hpp"
#include "Base/Constants.hpp"

namespace ECS
{
    /// @brief Chunks are allocated and owned by this structure, not archetypes.
    struct ChunkStore {
        static constexpr uint32_t BitmaskSize = 32;
        static constexpr uint32_t ChunkBunchCount = Constants::MaximumChunkCount / BitmaskSize;
    private:
        std::array<std::atomic<Chunk*>,Constants::MaximumChunkCount> chunks;
        std::array<std::atomic<uint32_t>,ChunkBunchCount> bitmasks;
    public:
        ChunkStore() = default;
        ~ChunkStore();
        Chunk* getChunkPointer(const ChunkIndex chunk);
        const Chunk* getChunkPointer(const ChunkIndex chunk) const;
        ChunkIndex allocateChunk();
        void freeChunk(const ChunkIndex chunk);
    };
} // namespace ECS


#endif
