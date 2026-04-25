#include "ECS/EntityQueryManager.hpp"
#include "ECS/Archetype.hpp"
#include "ECS/Base/ArchetypeQuery.hpp"

using namespace ECS;
bool EntityQueryManager::isMatchingArchetype(const Archetype& archetype, const ArchetypeQuery& query){
    const_span<ECS::TypeID> archetypeTypes = archetype.getType();
    if (archetypeTypes.size() < query.count[1] ||
        archetypeTypes.size() < query.count[2])
    {
        return false;
    }
    if(
        testMatchingArchetypeRequiredComponent(archetype.getType(),{query._all,query.count[1]}) &&
        testMatchingArchetypeOptionalComponent(archetype.getType(),{query._any,query.count[2]}) &&
        testMatchingArchetypeExcludedComponent(archetype.getType(),{query._none,query.count[3]})
    )
        return true;
    return false;
}
bool EntityQueryManager::testMatchingArchetypeRequiredComponent(const_span<TypeID> archetypeTypes, const_span<uint16_t> queryTypes){
    if (queryTypes.size() == 0)
        return true; // no types to search for
    uint32_t iNextQueryType = 0;
    uint32_t nextQueryType = queryTypes[iNextQueryType] & ArchetypeQuery::IndexMask;
    for (uint32_t i = 0; i < archetypeTypes.size(); i++)
    {
        uint32_t componentTypeIndex = archetypeTypes[i].index();
        if (unlikely(componentTypeIndex == nextQueryType))
        {
            if (++iNextQueryType == queryTypes.size())
                return true; // Ran out of query types; all required types were found.
            nextQueryType = queryTypes[iNextQueryType];
        }
        else if (componentTypeIndex > nextQueryType)
            return false; // A required type was not found
    }
    // Ran out of archetype types & didn't find all required types
    return false;
}
bool EntityQueryManager::testMatchingArchetypeOptionalComponent(const_span<TypeID> archetypeTypes, const_span<uint16_t> queryTypes){
    if (queryTypes.size() == 0)
        return true; // no types to search for
    uint32_t iNextQueryType = 0;
    uint32_t nextQueryType = queryTypes[iNextQueryType] & ArchetypeQuery::IndexMask;
    for (uint32_t i = 0; i < archetypeTypes.size(); i++)
    {
        uint32_t componentTypeIndex = archetypeTypes[i].index();
        while(componentTypeIndex > nextQueryType)
        {
            if (++iNextQueryType == queryTypes.size())
                return false; // Ran out of optional types and didn't find at least one of them
            nextQueryType = queryTypes[iNextQueryType];
        }
        if (unlikely(componentTypeIndex == nextQueryType))
            return true; // found at least one optional type
    }
    // Ran out of archetype types & didn't find all required types
    return false;
}
bool EntityQueryManager::testMatchingArchetypeExcludedComponent(const_span<TypeID> archetypeTypes, const_span<uint16_t> queryTypes){
    if (queryTypes.size() == 0)
        return true; // no types to search for
    uint32_t iNextQueryType = 0;
    uint32_t nextQueryType = queryTypes[iNextQueryType] & ArchetypeQuery::IndexMask;
    for (uint32_t i = 0; i < archetypeTypes.size(); i++)
    {
        uint32_t componentTypeIndex = archetypeTypes[i].index();
        while(componentTypeIndex > nextQueryType)
        {
            if (++iNextQueryType == queryTypes.size())
                return true; // Ran out of query types. No excluded types were found
            nextQueryType = queryTypes[iNextQueryType];
        }
        if (unlikely(componentTypeIndex == nextQueryType))
            return false; // An excluded type was found in archetype
    }
    // Ran out of archetype types & didn't find any excluded types
    return true;
}