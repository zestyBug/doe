#include "ECS/ListMap.hpp"
//#include "ECS/Archetype.hpp"
using namespace DOTS;

#include "ECS/Archetype.hpp"

void ListMap::init(int count)
{
    if (count < minimumSize())
        count = minimumSize();

    // is power of 2?
    if(0 != (count & (count - 1)))
        throw std::invalid_argument("Init(): count must be power of 2");

    hashes.resize(count);
    value.resize(count);

    emptyNodes = count;
    skipNodes = 0;
}

ListMap::Value ListMap::tryGetValue(uint32_t desiredHash)
{
    int32_t offset = desiredHash & hashMask();
    int attempts = 0;
    while (true)
    {
        uint32_t hash = hashes.at(offset);
        if (hash == 0)
            return invalideValue;
        if (hash == desiredHash)
        {
            return offset;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == size())
            return invalideValue;
    }
}

void ListMap::appendFrom(const ListMap& src)
{
    for (int offset = 0; offset < src.size(); ++offset)
    {
        uint32_t hash = src.hashes[offset];
        if (hash != 0 && hash != _SkipCode)
            add(src.hashes[offset], src.value[offset]);
    }
}
void ListMap::resize(int size)
{
    if (size < minimumSize())
        size = minimumSize();
    if (size == this->size())
        return;
    ListMap temp;
    temp.init(size);
    temp.appendFrom(*this);
    *this = std::move(temp);
}

uint32_t ListMap::add(uint32_t desiredHash, Value v)
{
    uint32_t offset = (desiredHash & hashMask());
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
        {
            hashes[offset] = desiredHash;
            value[offset] = v;
            //chunk.ListWithEmptySlotsIndex = offset;
            --emptyNodes;
            possiblyGrow();
            return offset;
        }

        if (hash == _SkipCode)
        {
            hashes[offset] = desiredHash;
            value[offset] = v;
            //chunk.ListWithEmptySlotsIndex = offset;
            --skipNodes;
            possiblyGrow();
            return offset;
        }

        offset = (offset + 1) & hashMask();
        ++attempts;
        if(attempts >= size())
            throw std::runtime_error("edd(): unable to find suitable location");
    }
}

void ListMap::remove(int32_t index)
{
    // throws, because you must use contains first
    if(index == -1)
        throw std::runtime_error("remove(): invalid index");
    hashes[index] = _SkipCode;
    ++skipNodes;
    possiblyShrink();
}

bool ListMap::contains(int32_t index)
{
    return index >= 0 && index < hashes.size();
}

int32_t ListMap::indexOf(uint32_t desiredHash)
{
    uint32_t offset = (desiredHash & hashMask());
    uint32_t attempts = 0;
    while (true)
    {
        uint32_t hash = hashes[offset];
        if (hash == 0)
            return -1;
        if (hash == desiredHash)
        {
            return offset;
        }
        offset = (offset + 1) & hashMask();
        ++attempts;
        if (attempts == size())
            return -1;
    }
}

void ListMap::removeByHash(uint32_t hash)
{
    int offset = indexOf(hash);
    if(offset == -1)
        throw std::runtime_error("remove(): invalid index");
    hashes.at(offset) = _SkipCode;
    ++skipNodes;
    possiblyShrink();
}