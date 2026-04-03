#include "ECS/ArchetypeChunkData.hpp"
using namespace ECS;


span<Version> ArchetypeChunkData::getChangeVersionArrayForType(uint32_t component_index)
{
    if(component_index >= this->componentCount)
        throw std::out_of_range("getChangeVersionArrayForType()");
    uint32_t index = component_index * this->_capacity;
    return span<Version>(_ChangeVersion+index,this->_count);
}
Version ArchetypeChunkData::getChangeVersion(uint32_t component_index, uint32_t index)
{
    if(index >= this->_count || component_index >= this->componentCount)
        throw std::out_of_range("getChangeVersion()");
    return _ChangeVersion[component_index * this->_capacity + index];
}
void ArchetypeChunkData::setChangeVersion(uint32_t component_index, uint32_t index, Version version)
{
    if(index >= this->_count || component_index >= this->componentCount)
        throw std::out_of_range("SetAllChangeVersion()");
    this->_ChangeVersion[(component_index * this->_capacity) + index] = version;
}
void ArchetypeChunkData::setAllChangeVersion(uint32_t index, Version version)
{
    if(index >= this->_count)
        throw std::out_of_range("SetAllChangeVersion()");
    for (uint32_t i = 1; i < this->componentCount; ++i)
        this->_ChangeVersion[(i * this->_capacity) + index] = version;
}

const_span<SharedComponentIndex> ArchetypeChunkData::getSharedComponentValueArrayForType(uint32_t shared_component_index_in_archtype){
    if(shared_component_index_in_archtype >= this->sharedComponentCount)
        throw std::out_of_range("getSharedComponentValueArrayForType()");
    return {_SharedComponentValue + (shared_component_index_in_archtype * _capacity), _capacity};
}
SharedComponentIndex ArchetypeChunkData::getSharedComponentValue(uint32_t shared_component_index_in_archtype, uint32_t index) {
    if(index >= this->_count || shared_component_index_in_archtype >= this->sharedComponentCount)
        throw std::out_of_range("getSharedComponentValue()");
    return _SharedComponentValue[shared_component_index_in_archtype * _capacity + index];
}
SharedComponentValues ArchetypeChunkData::getSharedComponentValues(uint32_t index)
{
    if(index >= this->_count)
        throw std::out_of_range("getSharedComponentValues()");
    return {_SharedComponentValue + index, _capacity * (uint32_t)sizeof(SharedComponentIndex)};
}
void ArchetypeChunkData::setSharedComponentValue(uint32_t shared_component_index_in_archtype, uint32_t index, SharedComponentIndex value)
{
    if(index >= this->_count || shared_component_index_in_archtype >= this->sharedComponentCount)
        throw std::out_of_range("setSharedComponentValue()");
    _SharedComponentValue[shared_component_index_in_archtype * _capacity + index] = value;
}

void ArchetypeChunkData::popBack() {
    if(this->_count < 1)
        throw std::out_of_range("popBack(): empty array");
    this->_count--;
}
void ArchetypeChunkData::add(Chunk* chunk, SharedComponentValues sharedComponentIndices, Version version) {
    if(this->_count >= this->_capacity)
        this->grow(this->_capacity < 1 ? 4 : this->_capacity*2);
    uint32_t index = this->_count++;
    _Chunk[index] = chunk;
    // New chunk, so all versions are reset.
    for (uint32_t i = 0; i < this->componentCount; i++)
        this->_ChangeVersion[(i * this->_capacity) + index] = version;
    for (uint32_t i = 0; i < sharedComponentCount; i++)
        _SharedComponentValue[(i * _capacity) + index] = sharedComponentIndices[i];
}
void ArchetypeChunkData::grow(uint32_t new_capacity) {
    if(new_capacity <= this->_capacity)
        throw std::invalid_argument("grow(): smaller new size");
    
    static_assert(sizeof(Version) == 4);
    /* WARN: DO NOT TOUCH THIS new_capacity * sizeof(Version) must be aligned by 64
     * to prevent false sharing, assuming sizeof(Version) == 4, new_capacity must be multiply of 16
     * so each component version can be updated by a seperate worker thread
     */
    new_capacity = (new_capacity+0xF)&0xFFFFFFF0;

    const uint32_t nextChunkIndexSize            = new_capacity * (uint32_t)sizeof(Chunk*);
    const uint32_t nextChangeVersionSize         = new_capacity * (uint32_t)sizeof(Version) * this->componentCount;
    const uint32_t nextSharedComponentValuesSize = new_capacity * (uint32_t)sizeof(SharedComponentIndex) * this->sharedComponentCount;
    const uint32_t new_v_size = nextChunkIndexSize + nextChangeVersionSize + nextSharedComponentValuesSize;

    align_ptr<uint8_t[]> new_data = make_align<uint8_t[]>(new_v_size);
    Chunk**    nextChunk;
    Version* nextChangeVersion;
    SharedComponentIndex*     nextSharedComponentValue;
    {
        uint8_t* nextBufferPtr   = new_data.get();
        nextChunk                = (Chunk**)nextBufferPtr;
        nextBufferPtr += nextChunkIndexSize;
        nextChangeVersion        = (Version*)nextBufferPtr;
        nextBufferPtr += nextChangeVersionSize;
        nextSharedComponentValue = (SharedComponentIndex*)nextBufferPtr;
    }

    if(this->buck) {
        memcpy(nextChunk,
            this->_Chunk,
            this->_count * sizeof(uint32_t)
        );
        for(uint32_t i = 0; i < componentCount; ++i)
            memcpy(nextChangeVersion + i * new_capacity,
                this->_ChangeVersion + i * this->_capacity,
                this->_count * sizeof(Version)
            );
        for(uint32_t i = 0; i < sharedComponentCount; ++i)
            memcpy(nextSharedComponentValue + i * new_capacity,
                this->_SharedComponentValue + i * this->_capacity,
                this->_count * sizeof(SharedComponentIndex)
            );
    }

    this->buck = std::move(new_data);
    this->_Chunk = nextChunk;
    this->_ChangeVersion = nextChangeVersion;
    this->_SharedComponentValue = nextSharedComponentValue;
    this->_capacity = new_capacity;
}
void ArchetypeChunkData::removeAtSwapBack(uint32_t index) {
    if (index >= this->_count)
        throw std::invalid_argument("removeAtSwapBack(): invalid index");
    if(this->_count < 1)
        throw std::out_of_range("removeAtSwapBack(): empty array");
    this->_count--;

    if (index == this->_count)
        return;

    _Chunk[index] = _Chunk[_count];
    // On *chunk order* change, no versions changed, just moved to new location.
    for (uint32_t i = 0; i < componentCount; i++)
        _ChangeVersion[(i * _capacity) + index] = _ChangeVersion[(i * _capacity) + _count];
    for (uint32_t i = 0; i < sharedComponentCount; i++)
        _SharedComponentValue[(i * _capacity) + index] = _SharedComponentValue[(i * _capacity) + _count];
}
