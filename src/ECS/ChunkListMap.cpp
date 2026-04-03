#include "ECS/ChunkListMap.hpp"
#include "ECS/Archetype.hpp"
using namespace ECS;
void ChunkListMap::add(Chunk* chunk) {
    auto sharedComponentValues = this->archetype->chunks.getSharedComponentValues(chunk->listIndex);
    uint32_t numSharedComponents = archetype->numSharedComponents();
    uint32_t desiredHash = getHashCode(sharedComponentValues,numSharedComponents);
    uint32_t offset = desiredHash & hashMask();
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
        {
            hashes[offset] = desiredHash;
            chunks[offset] = chunk;
            chunk->listWithEmptySlotsIndex = offset;
            --emptyNodes;
            possiblyGrow();
            return;
        }

        if (hash == _SkipCode)
        {
            hashes[offset] = desiredHash;
            chunks[offset] = chunk;
            chunk->listWithEmptySlotsIndex = offset;
            --emptyNodes;
            possiblyGrow();
            return;
        }

        if(unlikely(hash == desiredHash)) {
            if(chunks[offset] == chunk)
                throw std::invalid_argument("add(): adding duplicated item");
        }

        offset = (offset + 1) & hashMask();
        ++attempts;
        if(attempts >= capacity())
            // we should nor reach here, a possiblyGrow() call must prevent it
            throw std::runtime_error("add(): something went wrong");
    }
}
void ChunkListMap::remove(Chunk* chunk){
    int32_t offset = chunk->listWithEmptySlotsIndex;
    chunk->listWithEmptySlotsIndex = -1;
    if(offset >= this->_capacity || 0 > offset || chunks[offset] != chunk)
        throw std::invalid_argument("remove(): invalid chunk");
    hashes[offset] = _SkipCode;
    ++emptyNodes;
    possiblyShrink();
}
Chunk* ChunkListMap::tryGet(const SharedComponentValues sharedComponentValues, uint32_t numSharedComponents) const {
    uint32_t desiredHash = getHashCode(sharedComponentValues, numSharedComponents);
    uint32_t offset = desiredHash & hashMask();
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
            return nullptr;
        if (hash == desiredHash)
        {
            Chunk* chunk = chunks[offset];
            if(chunk == nullptr)
                throw std::runtime_error("tryGet(): invalid chunk");
            SharedComponentValues components = this->archetype->chunks.getSharedComponentValues(chunk->listIndex);
            if (components.equalTo(sharedComponentValues,numSharedComponents))
                return chunk;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == capacity())
            return nullptr;
    }
}
bool ChunkListMap::contains(Chunk* chunk) const {
    int32_t offset = chunk->listWithEmptySlotsIndex;
    return 0 <= offset && (uint32_t)offset < _capacity && chunks[offset] == chunk;
}