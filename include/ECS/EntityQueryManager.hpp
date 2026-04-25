#if !defined(ENTITYQUERYMANAGER_HPP)
#define ENTITYQUERYMANAGER_HPP

#include "cutil/basics.hpp"
#include "Base/TypeID.hpp"

namespace ECS {
    struct Archetype;
    struct ArchetypeQuery;
    struct EntityQueryManager {
        static bool isMatchingArchetype(const Archetype& archetype, const ArchetypeQuery& query);
    private:
        static bool testMatchingArchetypeRequiredComponent(const_span<TypeID> archetypeTypes, const_span<uint16_t> queryTypes);
        static bool testMatchingArchetypeOptionalComponent(const_span<TypeID> archetypeTypes, const_span<uint16_t> queryTypes);
        static bool testMatchingArchetypeExcludedComponent(const_span<TypeID> archetypeTypes, const_span<uint16_t> queryTypes);
    };
}

#endif // ENTITYQUERYMANAGER_HPP
