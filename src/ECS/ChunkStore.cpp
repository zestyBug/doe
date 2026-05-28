#include "ECS/ChunkStore.hpp"
#include <memory>
#include "cutil/span.hpp"
using namespace ECS;

ChunkStore::~ChunkStore(){
    uint32_t bitmask;
    uint32_t s_index = 0;
    uint32_t b_index = 0;
    for(;b_index < ChunkBunchCount;b_index++){
        bitmask = bitmasks[b_index];
        if(bitmask == 0)
            continue;
        for(;s_index<BitmaskSize;s_index++)
            if ((bitmask & (1<<s_index))){
                Chunk* v = chunks[b_index * 32 + s_index];
                if(v)
                    allocator<Chunk>().deallocate(v,1);
            }
    }
};
Chunk* ChunkStore::getChunkPointer(const ChunkIndex chunk) {
    if(chunk > Constants::MaximumChunkCount)
        return nullptr;
    return chunks[chunk].load();
}
ChunkIndex ChunkStore::allocateChunk() {
    uint32_t bitmask;
    uint32_t buffer;
    uint32_t s_index = 0;
    uint32_t b_index = 0;
    for(;b_index < ChunkBunchCount;){
        bitmask = bitmasks[b_index].load(std::memory_order_acquire);
        if(bitmask == ~(uint32_t)0) {
            b_index++;
            continue;
        }
        for(;s_index<BitmaskSize;s_index++)
            if ((bitmask & (1<<s_index)) == 0)
                break;
        buffer = bitmask | (1<<s_index);
        if(bitmasks[b_index].compare_exchange_weak(bitmask, buffer, std::memory_order_acq_rel))
            break;
    }
    if(b_index >= ChunkBunchCount)
        throw std::runtime_error("AllocateChunk(): out of memory");
    Chunk* v = allocator<Chunk>().allocate(1);
    buffer = b_index * BitmaskSize + s_index;
    chunks[buffer].store(v);
    v->index = ChunkIndex(buffer);
    return ChunkIndex(buffer);
}
void ChunkStore::freeChunk(const ChunkIndex chunk) {
    if(chunk > Constants::MaximumChunkCount)
        return;
    uint32_t s_index = chunk % BitmaskSize;
    uint32_t b_index = chunk / BitmaskSize;
    uint32_t bitmask = ~(1 << s_index);
    bitmasks[b_index] &= bitmask;
    Chunk* v = chunks[chunk].exchange(nullptr);
    if(v)
        allocator<Chunk>().deallocate(v,1);
    else
        throw std::invalid_argument("freeChunk(): invalid chunk");
}