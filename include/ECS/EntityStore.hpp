#if !defined(EntityStore_HPP)
#define EntityStore_HPP

#include <vector>
#include <memory>
#include <atomic>
#include "Base/Entity.hpp"
#include "Base/TypeID.hpp"
#include "Base/Chunk.hpp"
#include "Base/Constants.hpp"

namespace ECS
{
    class EntityStore {
        // MAGIC NUMBER
        static constexpr uint32_t EntitiesInBlock = 8192;
        static constexpr uint32_t BlockCount = 4096;
        static constexpr uint32_t MaximumTheoreticalAmountOfEntities = EntitiesInBlock * BlockCount;
        struct DataBlock
        {
            uint32_t allocated[EntitiesInBlock / 32];
            EntityInChunk entityInChunk[EntitiesInBlock];
            uint32_t versions[EntitiesInBlock];
            EntityName names[EntitiesInBlock];
            DataBlock() = default;
        };
        static constexpr uint32_t BlockSize = sizeof(DataBlock);
        static constexpr uint32_t BlockBusy = ~0;
        align_ptr<DataBlock>  dataBlocks[BlockCount];
        std::atomic<uint32_t> entityCount[BlockCount];

        void ExistsOrThrow(uint32_t blockIndex, uint32_t indexInBlock);
        void integrityCheck(uint32_t blockIndex);
    public:
        EntityStore() = default;
        ~EntityStore() = default;
        void integrityCheck();
        Chunk* getChunkIfExists(Entity entity);
        void setEntityInChunk(Entity entity, EntityInChunk entityInChunk);
        void setEntityVersion(Entity entity, uint32_t version);
        EntityInChunk getEntityInChunk(Entity entity);
        /// @brief Allocated and create some new Entities.
        /// @param entities output buffer. usually Chunk->buffer + index
        /// @param chunk if you want to fill EntityInChunk value
        /// @param firstEntityInChunkIndex if you want to fill EntityInChunk value
        void allocateEntities(span<Entity> entities, Chunk *chunk = nullptr, uint32_t firstEntityInChunkIndex = 0);
        void deallocateEntities(span<Entity> entities);
        const EntityName* getEntityName(Entity entity);
        void setEntityName(Entity entity, EntityName* name = nullptr);
    };
} // namespace ECS


#endif
