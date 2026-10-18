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
        struct DataBlock
        {
            uint32_t allocated         [Constants::EntityStoreBlockSize / 32];
            EntityInChunk entityInChunk[Constants::EntityStoreBlockSize];
            uint32_t versions          [Constants::EntityStoreBlockSize];
            EntityName names           [Constants::EntityStoreBlockSize];
            DataBlock() = default;
        };
        static constexpr uint32_t BlockSize = sizeof(DataBlock);
        static constexpr uint32_t BlockBusy = ~0;
        align_ptr<DataBlock>  dataBlocks [Constants::EntityStoreBlockCount];
        std::atomic<uint32_t> entityCount[Constants::EntityStoreBlockCount];

        void ExistsOrThrow(uint32_t blockIndex, uint32_t indexInBlock) const;
        uint32_t integrityCheck(uint32_t blockIndex) const;
    public:
        static constexpr uint32_t MaximumTheoreticalAmountOfEntities = Constants::EntityStoreBlockSize * Constants::EntityStoreBlockCount;
        EntityStore() = default;
        ~EntityStore() = default;
        uint32_t integrityCheck() const;
        Chunk* getChunkIfExists(Entity entity);
        const Chunk* getChunkIfExists(Entity entity) const;
        void setEntityInChunk(Entity entity, EntityInChunk entityInChunk);
        void setEntityVersion(Entity entity, uint32_t version);
        EntityInChunk getEntityInChunk(Entity entity) const;
        /// @brief Allocated and create some new Entities.
        /// @param entities output buffer. usually Chunk->buffer + index
        /// @param chunk if you want to fill EntityInChunk value
        /// @param firstEntityInChunkIndex if you want to fill EntityInChunk value
        void allocateEntities(span<Entity> entities, Chunk *chunk = nullptr, uint32_t firstEntityInChunkIndex = 0);
        /// @brief Deallocate entities from the list.
        /// @warning This function is not responsible for any linked chunk.
        void deallocateEntities(span<Entity> entities);
        const EntityName* getEntityName(Entity entity);
        void setEntityName(Entity entity, EntityName* name = nullptr);
        /// @note Use getChunkIfExists instead.
        bool exists(Entity entity) const;
    };
} // namespace ECS


#endif
