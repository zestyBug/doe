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
#include "SharedComponentStore.hpp"

class Test;

namespace ECS
{
    // the class that holds all entities
    struct EntityComponentStore final {
        friend class ChunkJobFunction;
        friend class ::Test;
        friend class Archetype;
    private:
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        EntityStore entities{};
        ChunkStore chunks{};
        SharedComponentStore sharedComponents;
        ArchetypeListMap archetypeTypeMap{};
        std::vector<align_ptr<Archetype>,allocator<align_ptr<Archetype>>> archetypes;
        align_ptr<Version[]> componentTypeOrderVersion;
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
            return this->archetypeTypeMap.tryGet(types);
        }
    public:
        /// @param types sorted array of types
        Archetype* getOrCreateArchetype(const_span<TypeID> types);
        Archetype* getArchetype(Entity entity);
    private:
        inline void setArchetype(Chunk *chunk, Archetype *arch){chunk->archetype = arch;}
        Archetype* getArchetype(ChunkIndex chunk);
        Archetype* getArchetype(Chunk* chunk);

        // Chunk and Batch
    private:
        Chunk *allocateChunk();
        inline void freeChunk(Chunk *chunk) {chunks.freeChunk(chunk->index);}
        Chunk* getCleanChunk(Archetype* archetype, SharedComponentValues sharedComponentValues);
        Chunk* getChunkWithEmptySlots(Archetype* archetype, SharedComponentValues sharedComponents);
        void setSharedComponentDataIndexForChunk(Chunk* chunk, Archetype* chunkArchetype, TypeID type, SharedComponentIndex value);
        inline Chunk* getChunk(Entity entity){
            return entities.getEntityInChunk(entity).chunk;
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
        void createEntities(Archetype* archetype, span<Entity> entities, SharedComponentValues values = SharedComponentValues());
        bool exists(Entity entity);
        bool hasComponent(Entity entity, TypeID type);
        const EntityName* getName(Entity entity);
        void setName(Entity entity, const EntityName* name);
    private:
        void allocateEntities(Archetype* arch, Chunk *chunk, uint32_t baseIndex, uint count, Entity* outputEntities = nullptr);
        void deallocateDataEntitiesInChunk(EntityBatchInChunk batch);
        void deallocateManagedComponents(EntityBatchInChunk batch);
        void destroyBatch(EntityBatchInChunk batch);
        void freeEntities(Chunk* chunk);
    public:
        void validateEntities(span<Entity> entities);
        void destroyEntities(const_span<Entity> entities);
        void freeAllEntities(bool resetVersion);
    private:
        void addExistingEntitiesInChunk(Chunk* chunk);
        inline void setEntityInChunk(Entity entity, EntityInChunk entityInChunk){
            entities.setEntityInChunk(entity, entityInChunk);
        }
        inline EntityInChunk getEntityInChunk(Entity entity) {
            return entities.getEntityInChunk(entity);
        }

        // Component
    public:
        SharedComponentIndex getSharedComponentDataIndex(Entity entity, TypeID typeIndex);
        const void* getComponentDataWithTypeRO(Entity entity, TypeID typeIndex);
        void* getComponentDataWithTypeRW(Entity entity, TypeID typeIndex);
    private:
        void moveAllSharedComponents(EntityComponentStore* srcEntityComponentStore);
        void incrementComponentOrderVersion(Archetype* archetype, SharedComponentValues sharedComponentValues);
        void incrementComponentTypeOrderVersion(const Archetype* archetype);

        // Move
    private:
        Archetype* getArchetypeWithAddedComponent(Archetype* archetype, TypeID type, uint32_t* indexInTypeArray = nullptr);
        /// @param types sorted
        Archetype* getArchetypeWithAddedComponents(Archetype* srcArchetype, const_span<TypeID> types);
        Archetype* getArchetypeWithRemovedComponent(Archetype* archetype, TypeID type, uint32_t* indexInOldTypeArray = nullptr);
        /// @param types sorted
        Archetype* getArchetypeWithRemovedComponents(Archetype* archetype, const_span<TypeID> types);
    private:
        void moveAndSetChangeVersion(EntityBatchInChunk batch, Archetype *archetype, SharedComponentValues sharedComponentValues, TypeID type);
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
        void move(Entity entity, Archetype* archetype, SharedComponentValues sharedComponentValues);
        /// @brief moves a entire chunk into another archetype/shared value
        /// @details calls move(EntityBatchInChunk , Archetype*, SharedComponentValues);
        /// @param chunk source chunk
        /// @param archetype destination archetype
        /// @param sharedComponentValues a pointer to the shared component values
        /// @return 
        void move(Chunk *chunk, Archetype* archetype, SharedComponentValues sharedComponentValues);
        /// @brief moves a batch of entities into another archetype/shared value
        /// @details calls move(EntityBatchInChunk, Chunk*)
        /// @param batch 
        /// @param archetype 
        /// @param sharedComponentValues 
        /// @return 
        void move(EntityBatchInChunk batch, Archetype* archetype, SharedComponentValues sharedComponentValues);
    public:
        bool addComponent(Entity entity, TypeID type);
        /// @param types sorted
        void addComponents(Entity entity, const_span<TypeID> types);
        bool removeComponent(Entity entity, TypeID type);
        /// @param types sorted
        void removeComponents(Entity entity, const_span<TypeID> types);
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
    };
} // namespace ECS


#endif
