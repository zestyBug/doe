#include "ECS/ArchetypeListMap.hpp"
#include "ECS/Archetype.hpp"

using namespace ECS;

void ArchetypeListMap::init(uint32_t count){
    if (count < minimumSize())
        count = minimumSize();

    // is power of 2?
    if(0 != (count & (count - 1)))
        throw std::invalid_argument("Init(): count must be power of 2");

    const uint32_t size1 = alignPointerSize(sizeof(Archetype*)*count);
    const uint32_t size2 = sizeof(uint32_t)*count;
    uint8_t* ptr = std::allocator<uint8_t>().allocate(size1+size2);
    pointers.reset((Archetype**)ptr);
    hashes = (uint32_t *)(ptr+size1);
    memset(hashes,0,size2);
    _capacity = count;
    emptyNodes = count;
}
void ArchetypeListMap::appendFrom(ArchetypeListMap& src){
    for (uint32_t offset = 0; offset < src.size(); ++offset)
    {
        uint32_t hash = src.hashes[offset];
        if (hash != 0 && hash != _SkipCode)
            add(src.pointers[offset]);
    }
    src.reset();
}
void ArchetypeListMap::resize(uint32_t size){
    if (size < minimumSize())
        size = minimumSize();
    if (size == this->size())
        return;
    ArchetypeListMap temp;
    temp.init(size);
    temp.appendFrom(*this);
    *this = std::move(temp);
}
uint32_t ArchetypeListMap::getHashCode(const_span<TypeID> type)
{
    uint32_t result = HashHelper::FNV1A32(type);
    if (result == 0 || result == _SkipCode)
        result = _AValidHashCode;
    return result;
}
void ArchetypeListMap::add(Archetype* ptr) {
    if(ptr == nullptr)
        throw std::invalid_argument("add(): null pointer");
    uint32_t desiredHash = getHashCode(ptr->getTypes());
    uint32_t offset = desiredHash & hashMask();
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
        {
            hashes[offset] = desiredHash;
            pointers[offset] = ptr;
            --emptyNodes;
            possiblyGrow();
            return;
        }

        if (hash == _SkipCode)
        {
            hashes[offset] = desiredHash;
            pointers[offset] = ptr;
            --emptyNodes;
            possiblyGrow();
            return;
        }

        if(unlikely(hash == desiredHash)) {
            if(pointers[offset] == ptr)
                throw std::invalid_argument("add(): adding duplicated item");
        }

        offset = (offset + 1) & hashMask();
        ++attempts;
        if(attempts >= size())
            // we should nor reach here, a possiblyGrow() call must prevent it
            throw std::runtime_error("add(): something went wrong");
    }
}
void ArchetypeListMap::remove(Archetype* ptr){
    int32_t offset = indexOf(ptr);
    if(offset < 0)
        throw std::runtime_error("remove(): pointer not found");
    hashes[offset] = _SkipCode;
    ++emptyNodes;
    possiblyShrink();
}
Archetype* ArchetypeListMap::tryGet(const_span<ECS::TypeID> key) const {
    uint32_t desiredHash = getHashCode(key);
    uint32_t offset = desiredHash & hashMask();
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
            return nullptr;
        if (hash == desiredHash)
        {
            Archetype *ptr = pointers[offset];
            if (ptr->getTypes() == key)
                return ptr;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == size())
            return nullptr;
    }
}
int32_t ArchetypeListMap::indexOf(Archetype* ptr) const {
    if(ptr == nullptr)
        return -1;
    uint32_t desiredHash = getHashCode(ptr->getTypes());
    uint32_t offset = desiredHash & hashMask();
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
            return -1;
        if (hash == desiredHash)
        {
            if (pointers[offset] == ptr)
                return offset;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == size())
            return -1;
    }
}