#if !defined(EntityComponentManager_HPP)
#define EntityComponentManager_HPP

#include "Base/Chunk.hpp"
#include "Base/Entity.hpp"
#include <vector>
#include <array>
#include <memory>
#include "Archetype.hpp"
#include "cutil/bitset.hpp"
#include "cutil/span.hpp"
#include "ArchetypeListMap.hpp"
#include "EntityStore.hpp"
#include "ChunkStore.hpp"
//#include "ResourceMap.hpp"

class Test;

namespace ECS
{
    // the class that holds all entities
    struct EntityComponentStore {
        friend class ChunkJobFunction;
        friend class ::Test;
    public:
    protected:
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        EntityStore entityStore{};
        ChunkStore chunkStore{};
        /// @brief hash does not include "Entity" component
        ArchetypeListMap archetypeTypeMap{};
        std::vector<align_ptr<Chunk>> chunksData{};
        int32_t FreeEntityIndex = Entity::Null;
        int32_t FreeArchetypeIndex = -1;
        // global version buffer, used for any entity create/modify command
        Version globalVersion = 1;


        //int m_UnmanagedSharedComponentCount;
        //std::vector<ComponentTypeList> m_UnmanagedSharedComponentsByType;
        // struct SharedComponentInfo
        // {
        //     int RefCount;
        //     int ComponentType;
        //     int Version;
        //     int HashCode;
        // };
        //std::vector<TypeID> m_UnmanagedSharedComponentTypes;
        //std::vector<std::vector<SharedComponentInfo>> m_UnmanagedSharedComponentInfo;

        //ResourceMap hashLookup;

        static mark_ptr<Archetype> createArchetype(const_span<TypeID> types);
    public:
        void freeAllEntities(bool resetVersion);
        void freeEntities(ChunkIndex chunk);
        const char* getName(Entity entity);
        void setName(Entity entity, const char* name);
        Archetype* GetArchetype(Entity entity);
        ChunkIndex GetChunk(Entity entity);
        /// @brief seaarchin for a given type
        /// @param type sorted array of types
        /// @return nullptr or a pointer to the archtype
        Archetype* getExistingArchetype(const_span<TypeID> types);
        Archetype* CreateArchetype(const_span<TypeID> types);
        uint32_t CountEntities();
        void incrementComponentOrderVersion(Archetype* archetype, SharedComponentValues sharedComponentValues);

        bool addComponent(Entity entity, TypeID type);
        /// @param componentTypeSet sorted
        void addComponent(Entity entity, const_span<TypeID> componentTypeSet);
        bool RemoveComponent(Entity entity, TypeID type);
        /// @param componentTypeSet sorted
        void RemoveComponent(Entity entity, const_span<TypeID> componentTypeSet);

        static inline uint32_t getComponentArraySize(uint32_t componentSize, uint32_t entityCount) {
            return alignTo64(componentSize, entityCount);
        }
        static uint32_t calculateSpaceRequirement(const_span<uint16_t> componentSizes, uint32_t entityCount)
        {
            uint32_t size = 0;
            for (const auto& componentSize : componentSizes)
                size += getComponentArraySize(componentSize, entityCount);
            return size;
        }
        static uint32_t calculateChunkCapacity(const_span<uint16_t> componentSizes, uint32_t bufferSize)
        {
            uint32_t totalSize = 0;
            for (const auto& componentSize : componentSizes)
                totalSize += componentSize;
            uint32_t capacity = bufferSize / totalSize;
            while (calculateSpaceRequirement(componentSizes, capacity) > bufferSize)
                --capacity;
            return capacity;
        }
    };
} // namespace ECS


#endif
