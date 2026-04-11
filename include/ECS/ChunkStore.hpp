#if !defined(ChunkStore_HPP)
#define ChunkStore_HPP

#include <array>
#include <memory>
#include <atomic>
#include "cutil/span.hpp"
#include "Base/Chunk.hpp"

namespace ECS
{
    struct ChunkStore {
        // MAGIC NUMBER
        static constexpr uint32_t MaximumChunkCount = 0x100000;
        static constexpr uint32_t BitmaskSize = MaximumChunkCount / 32;
    private:
        std::array<std::atomic<Chunk*>,MaximumChunkCount> chunks;
        std::array<std::atomic<uint32_t>,BitmaskSize> bitmasks;
    public:
        ChunkStore() = default;
        ~ChunkStore(){
            uint32_t bitmask;
            uint32_t s_index = 0;
            uint32_t b_index = 0;
            for(;b_index < BitmaskSize;b_index++){
                bitmask = bitmasks[b_index];
                if(bitmask == 0)
                    continue;
                for(;s_index<32;s_index++)
                    if ((bitmask & (1<<s_index))){
                        Chunk* v = chunks[b_index * 32 + s_index];
                        if(v)
                            allocator<Chunk>().deallocate(v,1);
                    }
            }
        };
        Chunk* getChunkPointer(const ChunkIndex chunk) {
            if(chunk > MaximumChunkCount)
                return nullptr;
            return chunks[chunk].load();
        }
        ChunkIndex allocateChunk() {
            uint32_t bitmask;
            uint32_t buffer;
            uint32_t s_index = 0;
            uint32_t b_index = 0;
            for(;b_index < BitmaskSize;){
                bitmask = bitmasks[b_index].load(std::memory_order_acquire);
                if(bitmask == ~(uint32_t)0) {
                    b_index++;
                    continue;
                }
                for(;s_index<32;s_index++)
                    if ((bitmask & (1<<s_index)) == 0)
                        break;
                buffer = b_index * 32 + s_index;
                if(bitmasks[b_index].compare_exchange_weak(bitmask, buffer, std::memory_order_acq_rel))
                    break;
            }
            if(b_index >= BitmaskSize)
                throw std::runtime_error("AllocateChunk(): out of memory");
            Chunk* v = allocator<Chunk>().allocate(1);
            chunks[buffer].store(v);
            v->index = ChunkIndex(buffer);
            return ChunkIndex(buffer);
        }
        void freeChunk(const ChunkIndex chunk) {
            if(chunk > MaximumChunkCount)
                return;
            uint32_t s_index = chunk % 32;
            uint32_t b_index = chunk / 32;
            uint32_t bitmask = ~(1 << s_index);
            bitmasks[b_index] &= bitmask;
            Chunk* v = chunks[chunk].exchange(nullptr);
            if(v)
                allocator<Chunk>().deallocate(v,1);
            else
                throw std::invalid_argument("freeChunk(): invalid chunk");
        }
    };
} // namespace ECS


#endif
