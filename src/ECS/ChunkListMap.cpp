#include "ECS/ChunkListMap.hpp"
#include "ECS/Archetype.hpp"
using namespace ECS;
void ChunkListMap::init(Archetype* _archetype, uint32_t count){
    if (count < minimumSize())
        count = minimumSize();

    // is power of 2?
    if(0 != (count & (count - 1)))
        throw std::invalid_argument("init(): count must be power of 2");
    // listWithEmptySlotsIndex is a signed int
    if(count > INT32_MAX)
        throw std::invalid_argument("init(): too large map");
    const uint32_t size1 = alignPointerSize(sizeof(uint32_t)*count);
    const uint32_t size2 = sizeof(Chunk*)*count;
    uint8_t* ptr = std::allocator<uint8_t>().allocate(size1+size2);
    hashes.reset((uint32_t*)ptr);
    memset(hashes.get(),0,size1);
    chunks = (Chunk**)(ptr + size1);
    this->archetype = _archetype;
    _capacity = count;
    emptyNodes = count;
}
void ChunkListMap::appendFrom(ChunkListMap& src) {
    for (uint32_t offset = 0; offset < src.capacity(); ++offset)
    {
        uint32_t hash = src.hashes[offset];
        if (hash != 0 && hash != _SkipCode)
            add(src.chunks[offset]);
    }
}
// the whole popuse is to free space when object is unused but still in memory
void ChunkListMap::reset(){
    hashes.reset();
    chunks = nullptr;
    emptyNodes=0;
}
void ChunkListMap::resize(uint32_t size){
    if (size < minimumSize())
        size = minimumSize();
    if (size == this->capacity())
        return;
    ChunkListMap temp;
    temp.init(this->archetype, size);
    temp.appendFrom(*this);
    *this = std::move(temp);
}
uint32_t ChunkListMap::getHashCode(const SharedComponentValues sharedComponents, uint32_t numSharedComponents)
{
    if(sharedComponents.firstIndex == nullptr)
        throw std::invalid_argument("getHashCode(): nullptr");
    uint32_t result;
    if (sharedComponents.stride == sizeof(SharedComponentIndex))
    {
        result = HashHelper::FNV1A32(sharedComponents.firstIndex, numSharedComponents * sizeof(SharedComponentIndex));
    }
    else
    {
        SharedComponentIndex indexArray[numSharedComponents];
        for (uint32_t i = 0; i < numSharedComponents; ++i)
            indexArray[i] = sharedComponents[i];
        result = HashHelper::FNV1A32(indexArray, numSharedComponents * sizeof(SharedComponentIndex));
    }
    if (result == 0 || result == _SkipCode)
        result = _AValidHashCode;
    return result;
}
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
    if(0 > offset || (uint32_t)offset >= this->_capacity || chunks[offset] != chunk)
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