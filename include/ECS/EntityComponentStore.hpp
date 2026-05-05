#if !defined(EntityComponentManager_HPP)
#define EntityComponentManager_HPP

#include <vector>
#include <array>
#include <memory>
#include "cutil/span.hpp"
#include "Base/ChunkListChanges.hpp"
#include "Base/Entity.hpp"
#include "ArchetypeListMap.hpp"
#include "EntityStore.hpp"
#include "ChunkStore.hpp"
#include "SharedComponentStore.hpp"

class Test;

namespace ECS
{
    struct Chunk;
    struct Archetype;
    // the class that holds all entities
    struct EntityComponentStore final {
        friend class ::Test;
        friend struct Archetype;
        friend struct EntityQueryManager;
    private:
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        ArchetypeListMap typeLookup;
        std::vector<align_ptr<Archetype>,allocator<align_ptr<Archetype>>> archetypes;
        EntityStore entityStore{};
        // global version buffer, used for any entity create/modify command
        Version globalVersion = 1;
        ChunkListChanges chunkListChangesTracker;
        align_ptr<Version[]> componentTypeOrderVersion;
        SharedComponentStore sharedComponents;
        ChunkStore chunks{};
        /// @brief used by EntityQueryManager
        uint32_t previousArchetypeCount = 0;

        void cleanChangeList();

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

        // Archetype
    private:
        static void validateArchetype(const_span<TypeID> types);
        static uint32_t calculateSpaceRequirement(const_span<uint16_t> componentSizes, uint32_t entityCount);
        static uint32_t calculateChunkCapacity(const_span<uint16_t> componentSizes, uint32_t bufferSize);
        /// @param types sorted array of types
        Archetype* createArchetype(const_span<TypeID> types);
        /// @brief seaarchin for a given type
        /// @param types sorted array of types
        /// @return nullptr or a pointer to the archtype
        inline Archetype* getExistingArchetype(const_span<TypeID> types){
            return this->typeLookup.tryGet(types);
        }
    public:
        /// @param types sorted array of types
        Archetype* getOrCreateArchetype(const_span<TypeID> types);
        Archetype* getArchetype(Entity entity);
        span<Archetype*> getArchetypes();
    private:
        inline void setArchetype(Chunk *chunk, Archetype *arch){chunk->archetype = arch;}
        Archetype* getArchetype(ChunkIndex chunk);
        Archetype* getArchetype(Chunk* chunk);

        // Chunk and Batch
    private:
        Chunk *allocateChunk();
        inline void freeChunk(Chunk *chunk) {chunks.freeChunk(chunk->index);}
        Chunk* getCleanChunk(Archetype* archetype, const SharedComponentValues sharedComponentValues);
        Chunk* getChunkWithEmptySlots(Archetype* archetype, const SharedComponentValues sharedComponents);
        void setSharedComponentDataIndexForChunk(Chunk* chunk, Archetype* chunkArchetype, TypeID type, SharedComponentIndex value);
        inline Chunk* getChunk(Entity entity){
            return entityStore.getEntityInChunk(entity).chunk;
        }
        EntityBatchInChunk getFirstEntityBatchInChunk(const_span<Entity> entities);
        /// @brief Create a SharedComponent list based on the provided chunk, after changing the value of a shared component.
        /// @param chunk sourse chunk
        /// @param type the shared component type to modify
        /// @param value the desired new value
        /// @param result a pointer to where the new value should be writen
        /// @return true if success, false if no change is detected
        bool getArchetypeChunkFilterWithChangedSharedComponent(Chunk *chunk, TypeID type, SharedComponentIndex value, SharedComponentIndex *result);

        // Entity
    public:
        uint32_t countEntities();
        void createEntities(Archetype* archetype, span<Entity> entities, const SharedComponentValues values = SharedComponentValues());
        bool exists(Entity entity);
        bool hasComponent(Entity entity, TypeID type);
        const EntityName* getName(Entity entity);
        void setName(Entity entity, const EntityName* name);
    private:
        /// @brief Allocating some Entity's in entityStore
        /// @param arch 
        /// @param chunk 
        /// @param baseIndex 
        /// @param count 
        /// @param outputEntities 
        void allocateEntities(Archetype* arch, Chunk *chunk, uint32_t baseIndex, uint32_t count, Entity* outputEntities = nullptr);
        /// @brief A wrapper to deallocate components, entites and fill the space
        /// @details EntityComponentStore::deallocateManagedComponents, EntityStore::deallocateEntities, Archetype::copy
        void deallocateDataEntitiesInChunk(EntityBatchInChunk batch);
        /// @brief calling destructor for managed non-zero-sized components in a batch
        void deallocateManagedComponents(EntityBatchInChunk batch);
        /// @brief destroying a batch of entities.
        /// @details a wrapper to call arch->deallocate
        void destroyBatch(EntityBatchInChunk batch);
        /// @brief Free all entities in a chunk from EntityStore
        void freeEntities(Chunk* chunk);
    public:
        void validateEntities(span<Entity> entities);
        /// @brief A wrapper to call destroyBatch
        void destroyEntities(const_span<Entity> entities);
        void freeAllEntities(bool resetVersion);
    private:
        void addExistingEntitiesInChunk(Chunk* chunk);
        inline void setEntityInChunk(Entity entity, EntityInChunk entityInChunk){
            entityStore.setEntityInChunk(entity, entityInChunk);
        }
        inline EntityInChunk getEntityInChunk(Entity entity) {
            return entityStore.getEntityInChunk(entity);
        }

        // Component
    public:
        SharedComponentIndex getSharedComponentDataIndex(Entity entity, TypeID typeIndex);
        const void* getComponentDataWithTypeRO(Entity entity, TypeID typeIndex);
        void* getComponentDataWithTypeRW(Entity entity, TypeID typeIndex);
    private:
        void moveAllSharedComponents(EntityComponentStore* srcEntityComponentStore);
        void incrementComponentOrderVersion(Archetype* archetype, const SharedComponentValues sharedComponentValues);
        void incrementComponentTypeOrderVersion(const Archetype* archetype);
        void buildSharedComponentIndicesWithAddedComponents(Chunk* srcChunk, const Archetype* dstArchetype, SharedComponentIndex* outSharedComponentValues);
        void buildSharedComponentIndicesWithAddedComponent (Chunk* srcChunk, const Archetype* dstArchetype, uint32_t newTypeIndex, SharedComponentIndex value, SharedComponentIndex* outSharedComponentValues);
        void buildSharedComponentIndicesWithRemovedComponents(Chunk* srcChunk, const Archetype* dstArchetype, SharedComponentIndex* outSharedComponentValues);
        void buildSharedComponentIndicesWithRemovedComponent (Chunk* srcChunk, const Archetype* dstArchetype, uint32_t oldTypeIndex, SharedComponentIndex* outSharedComponentValues);

        // Move
    private:
        Archetype* getArchetypeWithAddedComponent(Archetype* archetype, TypeID type, uint32_t* indexInTypeArray = nullptr);
        /// @param types sorted
        Archetype* getArchetypeWithAddedComponents(Archetype* srcArchetype, const_span<TypeID> types);
        Archetype* getArchetypeWithRemovedComponent(Archetype* archetype, TypeID type, uint32_t* indexInOldTypeArray = nullptr);
        /// @param types sorted
        Archetype* getArchetypeWithRemovedComponents(Archetype* archetype, const_span<TypeID> types);
    private:
        void moveAndSetChangeVersion(EntityBatchInChunk batch, Archetype *archetype, const SharedComponentValues sharedComponentValues, TypeID type);
        /// @brief move subset of chunk data into another chunk.
        /// @remarks chunks can be of same archetype (but differ by shared component values).
        /// @details if the chunk be smaller than available space, it only copies partially from the end of entity batch.
        /// @return returns the number moved. Caller handles if less than indicated in srcBatch.
        uint32_t move(EntityBatchInChunk srcBatch, Chunk* dstChunk);
        /// @brief moves a entity into another archetype/shared value
        /// @details calls move(EntityBatchInChunk , Archetype*, SharedComponentValues);
        /// @param entity the entity to move
        /// @param archetype 
        /// @param sharedComponentValues 
        /// @return 
        void move(Entity entity, Archetype* archetype, const SharedComponentValues sharedComponentValues);
        /// @brief moves a entire chunk into another archetype/shared value
        /// @details calls move(EntityBatchInChunk , Archetype*, SharedComponentValues);
        /// @param chunk source chunk
        /// @param archetype destination archetype
        /// @param sharedComponentValues a pointer to the shared component values
        /// @return 
        void move(Chunk *chunk, Archetype* archetype, const SharedComponentValues sharedComponentValues);
        /// @brief moves a batch of entities into another archetype/shared value
        /// @details calls move(EntityBatchInChunk, Chunk*)
        /// @param batch 
        /// @param archetype 
        /// @param sharedComponentValues 
        /// @return 
        void move(EntityBatchInChunk batch, Archetype* archetype, const SharedComponentValues sharedComponentValues);
    public:
        bool addComponent(Entity entity, TypeID type);
        /// @param types sorted
        bool addComponents(Entity entity, const_span<TypeID> types);
        bool removeComponent(Entity entity, TypeID type);
        /// @param types sorted
        bool removeComponents(Entity entity, const_span<TypeID> types);
        /// @param value shared component index, ignored if type is not a shared component.
        bool addComponent(EntityBatchInChunk entityBatchInChunk, TypeID type, SharedComponentIndex value);
        bool removeComponent(EntityBatchInChunk entityBatchInChunk, TypeID type);
        /// @param types sorted
        bool addComponents(EntityBatchInChunk entityBatchInChunk, const_span<TypeID> types);
        /// @param types sorted
        bool removeComponents(EntityBatchInChunk entityBatchInChunk, const_span<TypeID> types);

    private:
        inline void incrementGlobalSystemVersion() {globalVersion.updateVersion();}
    public:
        inline Version getGlobalSystemVersion() const {return globalVersion;}
        EntityComponentStore();
        ~EntityComponentStore();
    };
} // namespace ECS


#endif
