#include "ECS/Archetype.hpp"
#include "ECS/EntityComponentStore.hpp"
using namespace ECS;

void Archetype::addToChunkList(Chunk* chunk, SharedComponentValues sharedComponentIndices, uint32_t changeVersion) {
    if(chunk == nullptr)
        throw std::invalid_argument("nullptr");
    chunk->listIndex = chunks._count;
    chunks.add(chunk, sharedComponentIndices, changeVersion);
}
void Archetype::removeFromChunkList(Chunk* chunk) {
    if(chunk == nullptr)
        throw std::invalid_argument("removeFromChunkList(): nullptr");
    int32_t chunkListIndex = chunk->listIndex;
    if(chunkListIndex < 0)
        throw std::invalid_argument("removeFromChunkList(): invalid chunk");
    chunks.removeAtSwapBack(chunkListIndex);
    Chunk* chunkThatMoved = chunks[chunkListIndex];
    chunkThatMoved->listIndex = chunkListIndex;
}
void Archetype::addToChunkListWithEmptySlots(Chunk* chunk){
    chunk->listWithEmptySlotsIndex = chunksWithEmptySlots.size();
    chunksWithEmptySlots.push_back(chunk);
}
void Archetype::removeFromChunkListWithEmptySlots(Chunk* chunk){
    int32_t index = chunk->listWithEmptySlotsIndex;
    if(index >= chunksWithEmptySlots.size() || 0 > index)
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    if(chunk != chunksWithEmptySlots[index])
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    Chunk* lastChunk = chunksWithEmptySlots.back();
    if (chunk != lastChunk)
    {
        lastChunk->listWithEmptySlotsIndex = index;
        chunksWithEmptySlots[index] = lastChunk;
    }
    chunksWithEmptySlots.pop_back();
}
void Archetype::emptySlotTrackingRemoveChunk(Chunk* chunk){
    if(this != chunk->archetype)
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    if (numSharedComponents() == 0)
        removeFromChunkListWithEmptySlots(chunk);
    else
        freeChunksBySharedComponents.remove(chunk);
}
void Archetype::emptySlotTrackingAddChunk(Chunk* chunk){
    if(this != chunk->archetype)
        throw std::invalid_argument("removeFromChunkListWithEmptySlots(): invalid chunk");
    if (numSharedComponents() == 0)
        addToChunkListWithEmptySlots(chunk);
    else
        freeChunksBySharedComponents.add(chunk);
}
Chunk* Archetype::getExistingChunkWithEmptySlots(SharedComponentValues sharedComponentValues){
    if (numSharedComponents() == 0)
    {
        if (!chunksWithEmptySlots.empty())
        {
            Chunk* chunk = chunksWithEmptySlots.back();
            if(chunk->entityCount >= this->chunkCapacity)
                throw std::runtime_error("getExistingChunkWithEmptySlots(): invalid chunk");
            return chunk;
        }
        return nullptr;
    }
    // note: will be ChunkIndex.Null if none available.
    return freeChunksBySharedComponents.tryGet(sharedComponentValues, numSharedComponents());
}