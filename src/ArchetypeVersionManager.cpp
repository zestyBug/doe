#include "ECS/ArchetypeVersionManager.hpp"
using namespace ECS;


span<version_t> ArchetypeVersionManager::getChangeVersionArrayForType(uint32_t component_index)
{
    if(component_index > this->componentCount)
        throw std::out_of_range("getChangeVersionArrayForType()");
    uint32_t index = component_index * this->_capacity;
    return span<version_t>(&_ChangeVersion(index),this->_count);
}
version_t& ArchetypeVersionManager::getChangeVersion(uint32_t component_index, uint32_t chunkIndex)
{
    auto changeVersions = getChangeVersionArrayForType(component_index);
    return changeVersions.at(chunkIndex);
}
void ArchetypeVersionManager::setAllChangeVersion(uint32_t chunkIndex, version_t version)
{
    if(chunkIndex > this->_count)
        throw std::out_of_range("SetAllChangeVersion()");
    for (uint32_t i = 1; i < this->componentCount; ++i)
        _ChangeVersion((i * this->_capacity) + chunkIndex) = version;
}
void ArchetypeVersionManager::popBack(){
    if(this->_count<1)
        throw std::out_of_range("removeAtSwapBack(): empty array");
    this->_count--;
}
void ArchetypeVersionManager::add(version_t version){
    uint32_t index = this->_count++;
    if(index>=this->_capacity)
        this->grow(this->_capacity<1 ? 4 : this->_capacity*2);

    // New chunk, so all versions are reset.
    for (uint32_t i = 0; i < this->componentCount; i++)
        this->_ChangeVersion((i * this->_capacity) + index) = version;
}
void ArchetypeVersionManager::grow(uint32_t new_capacity){
    if(new_capacity <= this->_capacity)
        throw std::invalid_argument("grow(): smaller new size");
    
    static_assert(sizeof(version_t) == 4);
    /* WARN: DO NOT TOUCH THIS new_capacity * sizeof(version_t) must be align 64
     * to prevent false sharing, assuming sizeof(version_t) == 4, new_capacity multiply of 16
     */
    new_capacity = (new_capacity+0xF)&0xFFFFFFF0;

    const size_t new_v_size = new_capacity * sizeof(version_t) * this->componentCount;

    uint8_t *new_data = allocator().allocate(new_v_size);

    if(this->data != nullptr && this->_count > 0){
        for(uint32_t i = 0; i < componentCount; ++i) {
            memcpy((version_t*)(new_data) + (i * new_capacity),
                &this->_ChangeVersion(i * this->_capacity),
                this->_count * sizeof(version_t));
        }
        allocator().deallocate(this->data);
    }

    this->data = new_data;
    this->_capacity = new_capacity;
}