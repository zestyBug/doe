#if !defined(EntityStore_HPP)
#define EntityStore_HPP

#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include "Base/Entity.hpp"
#include "Base/TypeID.hpp"
#include "Base/Chunk.hpp"

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

        void ExistsOrThrow(uint32_t blockIndex, uint32_t indexInBlock) {
            if(blockIndex >= BlockCount)
                throw std::invalid_argument("ExistsOrThrow(): entity does not exists");
            DataBlock* block = dataBlocks[blockIndex].get();
            if(block==nullptr)
                throw std::invalid_argument("ExistsOrThrow(): entity does not exists");
            if((block->allocated[indexInBlock / 32] & (1UL << (indexInBlock % 32)))==0)
                throw std::invalid_argument("ExistsOrThrow(): entity does not exists");
        }
        void integrityCheck(uint32_t blockIndex)
        {
            // It is assumed that integrity check is performed on a stable state.
            // In other words, no potential concurrent access.
            DataBlock* block = dataBlocks[blockIndex].get();
            if(block == nullptr) {
                if(0 != entityCount[blockIndex])
                    throw std::runtime_error("integrityCheck()");
                return;
            }
            uint32_t* allocated = block->allocated;
            uint32_t count = 0;
            for (uint32_t i = 0; i < EntitiesInBlock; ++i)
                count += (allocated[i / 32] >> (i % 32)) & 1;
            if(count != entityCount[blockIndex])
                throw std::runtime_error("integrityCheck()");
        }
    public:
        EntityStore() = default;
        ~EntityStore() = default;
        void integrityCheck()
        {
            for (uint32_t i = 0; i < BlockCount; i++)
            {
                integrityCheck(i);
            }
        }
        Chunk* getChunkIfExists(Entity entity)
        {
            uint32_t blockIndex   = entity.index() / EntitiesInBlock;
            uint32_t indexInBlock = entity.index() % EntitiesInBlock;
            if(blockIndex >= BlockCount)
                return nullptr;
            DataBlock* block = dataBlocks[blockIndex].get();
            if (block == nullptr)
                return nullptr;
            if (/*(entity.version() & 1) == 0 || */block->versions[indexInBlock] != entity.version())
                return nullptr;
            return block->entityInChunk[indexInBlock].chunk;
        }
        void setEntityInChunk(Entity entity, EntityInChunk entityInChunk)
        {
            uint32_t blockIndex   = entity.index() / EntitiesInBlock;
            uint32_t indexInBlock = entity.index() % EntitiesInBlock;
            ExistsOrThrow(blockIndex, indexInBlock);
            DataBlock* block = dataBlocks[blockIndex].get();
            ((EntityInChunk*)block->entityInChunk)[indexInBlock] = entityInChunk;
        }
        void setEntityVersion(Entity entity, uint32_t version)
        {
            uint32_t blockIndex   = entity.index() / EntitiesInBlock;
            uint32_t indexInBlock = entity.index() % EntitiesInBlock;
            ExistsOrThrow(blockIndex, indexInBlock);
            DataBlock* block = dataBlocks[blockIndex].get();
            block->versions[indexInBlock] = version;
        }
        EntityInChunk getEntityInChunk(Entity entity)
        {
            uint32_t blockIndex   = entity.index() / EntitiesInBlock;
            uint32_t indexInBlock = entity.index() % EntitiesInBlock;
            ExistsOrThrow(blockIndex, indexInBlock);
            DataBlock* block = dataBlocks[blockIndex].get();
            return block->entityInChunk[indexInBlock];
        }
        /// @brief Allocated and create some new Entities.
        /// @param entities output buffer. usually Chunk->buffer + index
        /// @param chunk if you want to fill EntityInChunk value
        /// @param firstEntityInChunkIndex if you want to fill EntityInChunk value
        void allocateEntities(span<Entity> entities, Chunk *chunk = nullptr, uint32_t firstEntityInChunkIndex = 0)
        {
            uint32_t entityInChunkIndex = firstEntityInChunkIndex;
            /// @brief the current chunk index we are searching for empty slots
            for (uint32_t i = 0; i < BlockCount; i++)
            {
                uint32_t blockCount = entityCount[i].load();
                if (blockCount == BlockBusy || blockCount == EntitiesInBlock) {continue;}
                /// the blocks available entities
                uint32_t blockAvailable = EntitiesInBlock - blockCount;
                /// number of entities to allocate in this block
                uint32_t count = std::min(blockAvailable, entities.size());
                // Set the count to a flag indicating that this block is busy (-1)
                uint32_t buffer = blockCount;
                if (!entityCount[i].compare_exchange_weak(buffer, BlockBusy)) {
                    // Another thread is messing around with this block, it's either busy or was changed
                    // between the time we read the count and now. In both cases, let's keep looking.
                    continue;
                }
                DataBlock* block = (DataBlock*)dataBlocks[i].get();
                // Be careful that the block might exist even if the count is zero, checking the pointer
                // for null is the only valid way to tell if the block exists or not.
                if (block == nullptr) {
                    dataBlocks[i] = make_align<DataBlock>();
                    block = dataBlocks[i].get();
                    new (block) DataBlock();
                }
                // a buffer variable
                uint32_t remainingCount = std::min(blockAvailable, count);
                uint32_t* allocated = block->allocated;
                uint32_t* versions = block->versions;
                EntityInChunk* entityInChunk = block->entityInChunk;
                uint32_t baseEntityIndex = i * EntitiesInBlock;

                while (remainingCount > 0)
                    for (uint32_t maskIndex = 0; maskIndex < EntitiesInBlock / 32; maskIndex++)
                        if (allocated[maskIndex] != ~0UL)
                        {
                            // There is some space in this one
                            for (int entity = 0; entity < 32; entity++)
                            {
                                uint32_t mask = 1UL << (entity % 32);
                                if ((allocated[maskIndex] & mask) == 0)
                                {
                                    uint32_t indexInBlock = maskIndex * 32 + entity;
                                    allocated[maskIndex] |= mask;
                                    uint32_t index = baseEntityIndex + indexInBlock;
                                    if(index > Entity::Maximum)
                                        throw std::runtime_error("allocateEntities(): out of entity index");
                                    entities[0] = Entity{(int32_t)index, versions[indexInBlock]++};
                                    if (chunk != nullptr)
                                        entityInChunk[indexInBlock] = EntityInChunk{chunk,entityInChunkIndex};
                                    else
                                        entityInChunk[indexInBlock] = EntityInChunk();
                                    ++entities;
                                    entityInChunkIndex++;
                                    remainingCount--;
                                    if (remainingCount == 0)
                                        break;
                                }
                            }
                            if (remainingCount == 0)
                                break;
                        }
                if(0 != remainingCount)
                    throw std::runtime_error("AllocateEntities()");
                buffer = BlockBusy;
                if(!entityCount[i].compare_exchange_weak(buffer, blockCount + count))
                    throw std::runtime_error("AllocateEntities()");
                if(entities.empty())
                    return;
            }
            throw std::runtime_error("AllocateEntities(): could not find a data block for entity allocation.");
        }
        void deallocateEntities(span<Entity> entities)
        {
            for (uint32_t i = 0; i < entities.size();)
            {
                uint32_t rangeStart = i;
                uint32_t startIndex = entities[i].index();
                uint32_t blockIndex = startIndex / EntitiesInBlock;
                if(blockIndex >= BlockCount)
                    throw std::invalid_argument("deallocateEntities(): entity does not exists");
                uint32_t prevIndexInBlock = startIndex % EntitiesInBlock;
                for (i++; i < entities.size(); i++)
                {
                    if (entities[i].index() / EntitiesInBlock != blockIndex)
                        // Different data block
                        break;
                    uint32_t indexInBlock = entities[i].index() % EntitiesInBlock;
                    if (indexInBlock != prevIndexInBlock + 1)
                        // Same data block, but not the next entity in range
                        break;
                    prevIndexInBlock = indexInBlock;
                }
                uint32_t rangeEnd = i;
                uint32_t endIndex = startIndex + rangeEnd - rangeStart;
                uint32_t blockCount = entityCount[blockIndex].load();

                if (blockCount == 0)
                    // Looks like this block has been already deallocated.
                    // We are trying to deallocate entities which are already gone, skip.
                    continue;

                while (true)
                {
                    if (blockCount != BlockBusy)
                    {
                        uint32_t buffer = blockCount;
                        // Set the count to a flag indicating that this block is busy (-1)
                        if (entityCount[blockIndex].compare_exchange_weak(buffer, BlockBusy))
                            // Exchange succeeded
                            break;
                        blockCount = buffer;
                    }
                    else
                        blockCount = entityCount[blockIndex].fetch_add(0);
                }

                if (blockCount == 0)
                {
                    // This is very unlikely, but the block has been deallocated while we were waiting for it.
                    // Same as the test above, skip the block. But don't forget to restore the count.
                    uint32_t buffer = BlockBusy;
                    if (!entityCount[blockIndex].compare_exchange_weak(buffer, 0))
                        throw std::runtime_error("DeallocateEntities()");
                    continue;
                }

                DataBlock* block = dataBlocks[blockIndex].get();

                // It would be tempting to check to immediately check if deallocation would bring the entity count
                // for the data block to zero and deallocate the whole block. Unfortunately, in the eventuality that
                // we are trying to deallocate an entity which was already deallocated, this could lead to
                // discarding the data used by allocated entities. So we need to take the slow route and process
                // the data block even if we end up getting rid of it immediately after.

                uint32_t* allocated = block->allocated;
                uint32_t* versions = block->versions;

                for (uint32_t j = startIndex, indexInEntitiesArray = rangeStart; j < endIndex; j++, indexInEntitiesArray++)
                {
                    uint32_t indexInBlock = j % EntitiesInBlock;
                    if (versions[indexInBlock] == entities[indexInEntitiesArray].version())
                    {
                        // Matching versions confirm that we are deallocating the intended entity
                        uint32_t mask = 1UL << (indexInBlock % 32);
                        versions[indexInBlock]++;
                        allocated[indexInBlock / 32] &= ~0UL ^ mask;
                        blockCount--;
                    }
                }
                // Do not deallocate the block even if it's empty. Versions should be preserved.
                {
                    uint32_t buffer = BlockBusy;
                    if (!entityCount[blockIndex].compare_exchange_weak(buffer, blockCount))
                        throw std::runtime_error("DeallocateEntities()");
                }
            }
        }
        const EntityName* getEntityName(Entity entity)
        {
            uint32_t blockIndex   = entity.index() / EntitiesInBlock;
            uint32_t indexInBlock = entity.index() % EntitiesInBlock;
            ExistsOrThrow(blockIndex, indexInBlock);
            DataBlock* block = dataBlocks[blockIndex].get();
            return &block->names[indexInBlock];
        }
        void setEntityName(Entity entity, EntityName* name = nullptr)
        {
            uint32_t blockIndex   = entity.index() / EntitiesInBlock;
            uint32_t indexInBlock = entity.index() % EntitiesInBlock;
            ExistsOrThrow(blockIndex, indexInBlock);
            DataBlock* block = dataBlocks[blockIndex].get();
            if(name != nullptr)
                memcpy(block->names + indexInBlock, name, sizeof(EntityName));
            else
                memset(block->names + indexInBlock, 0, sizeof(EntityName));
        }
    };
} // namespace ECS


#endif
