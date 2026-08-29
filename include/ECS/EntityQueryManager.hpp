#if !defined(ENTITYQUERYMANAGER_HPP)
#define ENTITYQUERYMANAGER_HPP

#include "cutil/basics.hpp"
#include "cutil/static_array.hpp"
#include "Base/TypeID.hpp"
#include "Base/Constants.hpp"
#include "Base/Query.hpp"

namespace ECS {
    struct Archetype;
    struct Chunk;
    struct EntityComponentStore;
    struct EntityQueryBuilder;

    /**
     * Actual query data is stored here.
     * Including matching archetypes and a matching chunk list cache.
     */
    struct EntityQueryData {
        inline void invalidateCache(){
            validCache = false;
        }
        inline bool isValid(){
            return validCache;
        }
        ~EntityQueryData() = default;
    private:
        friend struct JobChunkWrapperBase;
        friend struct EntityQueryManager;
        friend struct ComponentDependencyManager;
        typedef const Archetype* ArchetypeCache;
        /// @brief matching archetypes.
        std::unique_ptr<ArchetypeCache[]> archetypes;
        /// @brief how to access: typesIndex[archetypeIndex(in this->archetypes) * this->firstNoneIndex + typeIndex(in the archetype)]
        /// @details -1 means not found
        int32_t             *typesIndex = nullptr;
        uint32_t             archetypesCapacity = 0;
        uint32_t             archetypesCount = 0;
        /// @brief matching chunks cache
        struct ChunkCache {
            const Chunk *value;
            /// @brief Archetype index in this->archetypes.
            uint32_t archetypeIndex;
        };
        std::unique_ptr<ChunkCache[]>  cache;
        uint32_t             cacheCapacity = 0;
        uint32_t             cacheCount = 0;
        struct TypeQuery {
            static const uint16_t WriteFlag = 1;
            static const uint16_t AnyFlag = 1 << 1;
            static const uint16_t NoneFlag = 1 << 2;
            TypeID type;
            uint16_t flags;
            uint16_t parameterIndex;
            bool operator < (const TypeQuery& v){
                uint16_t f1 = this->flags & 0b110;
                uint16_t f2 =     v.flags & 0b110;
                if(f1 == f2){
                    if(this->type == v.type)
                        throw std::invalid_argument("TypeQuery: same repeated query");
                    return this->type < v.type;
                }
                return f1 < f2;
            }
        };
        /// @brief Queries order is as follow: All, Any, None
        std::unique_ptr<TypeQuery[]> queries;
        uint32_t queryCount = 0;
        uint32_t firstAnyIndex = 0;
        /// @brief can be used as total number of non-zero sized components in the query.
        /// @note archetypes may lack some of the optional components.
        uint32_t firstNoneIndex = 0;
        uint32_t validCache = false;
        // simply an index.
        uint32_t ID;
    };
    /**
     * This is a helper class to create and keep queries caches up to date.
     * To manage new new queries and new archetype
     */
    struct EntityQueryManager { 
    private:
        static_array<EntityQueryData,Constants::MaximumQueryCount> entityQueryDatas;
        static bool testMatchingArchetypeRequiredComponent(const_span<TypeID> archetypeTypes, const_span<EntityQueryData::TypeQuery> queryTypes);
        static bool testMatchingArchetypeOptionalComponent(const_span<TypeID> archetypeTypes, const_span<EntityQueryData::TypeQuery> queryTypes);
        static bool testMatchingArchetypeExcludedComponent(const_span<TypeID> archetypeTypes, const_span<EntityQueryData::TypeQuery> queryTypes);
    public:
        EntityQueryManager(){}
        EntityQueryImpl createEntityQuery(const EntityQueryBuilder&, EntityComponentStore &ecs);
        static void addArchetypeIfMatching(Archetype *archetype, EntityQueryData &query);
        void addAdditionalArchetypes(span<Archetype*> archetypeList);
        static void rebuildMatchingChunkCache(EntityQueryData &query);
        void updateNewArchetypes(EntityComponentStore &ecs);
        void iterate(EntityComponentStore &, EntityQueryImpl, void (*)(span<const void*>));
    };
}

#endif // ENTITYQUERYMANAGER_HPP
