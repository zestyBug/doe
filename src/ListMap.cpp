#include "ECS/ListMap.hpp"
//#include "ECS/Archetype.hpp"
using namespace DOTS;

#include "ECS/Archetype.hpp"



void ArchetypeListMap::setCapacity(uint32_t capacity)
{
    if (capacity < minimumSize())
        capacity = minimumSize();
    hashes.resize(capacity);
    archetypes.resize(capacity);
}

void ArchetypeListMap::init(uint32_t count)
{
    if (count < minimumSize())
        count = minimumSize();

    // is power of 2?
    if(0 != (count & (count - 1)))
        throw std::invalid_argument("Init(): count must be power of 2");

    hashes.resize(count);
    archetypes.resize(count);

    emptyNodes = count;
    skipNodes = 0;
}

Archetype* ArchetypeListMap::tryGet(span<TypeIndex> types) const
{
    if(types.size() == 0)
        return nullptr;
    uint32_t desiredHash = getHashCode(types);
    uint32_t offset = desiredHash & hashMask();
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes.at(offset);
        if (hash == 0)
            return nullptr;
        if (hash == desiredHash)
        {
            Archetype *archetype = archetypes[offset];
            // WARN: first component must be Entity
            const auto dropFirst = archetype->types + 1;
            if (dropFirst == types)
                return archetype;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == size())
            return nullptr;
    }
}

Archetype* ArchetypeListMap::get(span<TypeIndex> types) const
{
    Archetype* result = tryGet(types);
    if(result == nullptr)
        throw std::runtime_error("get(): found no suitable type");
    return result;
}

void ArchetypeListMap::appendFrom(ArchetypeListMap& src)
{
    for (uint32_t offset = 0; offset < src.size(); ++offset)
    {
        uint32_t hash = src.hashes[offset];
        if (hash != 0 && hash != _SkipCode)
            add(src.archetypes[offset]);
    }
    src.dispose();
}
void ArchetypeListMap::resize(uint32_t size)
{
    if (size < minimumSize())
        size = minimumSize();
    if (size == this->size())
        return;
    ArchetypeListMap temp;
    temp.init(size);
    temp.appendFrom(*this);
    *this = std::move(temp);
}

void ArchetypeListMap::add(Archetype* archetype)
{
    if(archetype == nullptr)
        throw std::invalid_argument("add(): null archtype");
    uint32_t desiredHash = getHashCode(archetype->types+1);
    uint32_t offset = (int)(desiredHash & hashMask());
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes.at(offset);
        if (hash == 0)
        {
            hashes[offset] = desiredHash;
            archetypes[offset] = archetype;
            --emptyNodes;
            possiblyGrow();
            return;
        }

        if (hash == _SkipCode)
        {
            hashes[offset] = desiredHash;
            archetypes[offset] = archetype;
            --skipNodes;
            possiblyGrow();
            return;
        }

        offset = (offset + 1) & hashMask();
        ++attempts;
        if(attempts >= size())
            // we should nor reach here, a possiblyGrow() call must prevent it
            throw std::runtime_error("add(): something wet wrong");
    }
}

void ArchetypeListMap::remove(Archetype* archetype)
{
    int32_t offset = indexOf(archetype);
    if(offset != -1)
        throw std::runtime_error("remove(): archtype not found");
    hashes[offset] = _SkipCode;
    ++skipNodes;
    possiblyShrink();
}

bool ArchetypeListMap::contains(Archetype* archetype) const
{
    return indexOf(archetype) != -1;
}

int32_t ArchetypeListMap::indexOf(Archetype* archetype) const
{
    if(archetype == nullptr)
        return -1;
    uint32_t desiredHash = getHashCode(archetype->types);
    uint32_t offset = (desiredHash & hashMask());
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
            return -1;
        if (hash == desiredHash)
        {
            Archetype *c = archetypes[offset];
            if (c == archetype)
                return offset;
            return offset;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == size())
            return -1;
    }
}
