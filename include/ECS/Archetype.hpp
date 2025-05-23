#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

#include <stdint.h>
#include <vector>
#include <array>
#include <utility>
#include "defs.hpp"
#include "ArchetypeChunkData.hpp"
#include "basics.hpp"

namespace DOTS
{
    struct Chunk {
        void *memory = nullptr;
        Chunk() = default;
        Chunk(const Chunk& ) = delete;
        Chunk& operator = (const Chunk& ) = delete;
        Chunk& operator = (Chunk&& obj){
            if(this != &obj){
                this->memory = obj.memory;
                obj.memory = nullptr;
            }
            return *this;
        }
        Chunk(Chunk&& obj) {
            *this = std::move(obj);
        }
        ~Chunk() {
            allocator().deallocate(this->memory);
            this->memory = nullptr;
        }

        static const int32_t _BufferOffset = 64; // (must be cache line aligned)
        static const int _ChunkSize = 16 * 1024;
        static const int _BufferSize = _ChunkSize - _BufferOffset;
        static const int _MaximumEntitiesPerChunk = 1024;
    };

    class Archetype final
    {
    protected:

        friend class ArchetypeListMap;
        friend class Register;
        // maximum number of entities that can be fit into a single chunk
        uint32_t chunkCapacity = 0;
        uint32_t lastChunkEntityCount = 0;
        // note: chunks are united, means
        // if we remove a entity for a random chunk,
        // last entity on last chunk will be
        // filled in it place so iterating more efficiently.
        std::vector<Chunk> chunksData{};
        ArchetypeChunkData chunks{};


        // optimal for 16 component per archtype or less
        // Entity are istored as first type
        span<TypeIndex> types{};
        // faster access to TypeIndex::realIndecies() for iteration
        span<uint16_t> realIndecies{};
        span<uint32_t> offsets{};
        span<uint16_t> sizeOfs{};
        // any index above/equal this is a tag and it size is equal to 0
        // firstTagIndex == 1 all tag, firstTagIndex == types.size() no tag
        uint16_t firstTagIndex = 0;
        uint16_t flags=0;
        uint32_t archetypeIndex=0;
        Archetype(/* args */) = default;
    public:
        ~Archetype(/* args */) {
        }

        // invalid index and max entity number
        static constexpr uint32_t nullEntityIndex = 0xfffffff;
        static constexpr uint32_t nullChunkIndex = 0xfffffff;
        static constexpr uint32_t maxEntityPerChunk = 0x555;
        

        Archetype& operator =(const Archetype&) = delete;
        Archetype(const Archetype&) = delete;
        // data structor stores pointer to itself
        // any move operation requires recalculation
        Archetype(Archetype&&) = delete;

        // estimated capacity
        // check before accessing an unallocated archtype
        uint32_t capacity() const {
            return chunkCapacity * chunks.capacity();
        }
        bool empty() const {
            return this->chunks.empty();
        }
        // this fucntion is a little more expensive than empty
        uint32_t count() const {
            const uint32_t v = this->chunksData.size();
            if(v < 1)
                return 0;
            else
                return (v-1) * chunkCapacity + lastChunkEntityCount;
        }

        uint16_t nonZeroSizedTypesCount() const {
            return firstTagIndex;
        }

        static Archetype* createArchetype(span<TypeIndex> types);

        /// @brief find or create an index on chunks list.
        /// @param e
        /// @return index within the archetype
        uint32_t createEntity();

        /// @brief remove an entity from archtype and returns entity value of the entity that must replaced it
        uint32_t removeEntity(uint32_t entityIndex);

        // requesting component that does not exist results undefined behaviour

        /// @brief returns memory address of a given component and entity
        /// @param componentIndex component inside the archetype index
        /// @param entityIndex entity inside the archetype index
        /// @return returns pointer or exception
        void* getComponent(uint16_t componentIndex,uint32_t entityIndex);

        void* getChunkComponent(uint16_t componentIndex,size_t chunkIndex){
            uint8_t * const mem = (uint8_t*) this->chunksData.at(chunkIndex).memory;
            return mem + offsets.at(componentIndex);
        }

        // TODO: binary search?
        int32_t getIndex(TypeIndex t);
    protected:
        // the entity chunk index
        inline uint32_t getChunkIndex(const uint32_t i) const {
            return (i / this->chunkCapacity);
        }
        // index within a chunk
        inline uint32_t getInChunkIndex(const uint32_t i) const {
            return (i % this->chunkCapacity);
        }
        /// @brief alignes array size to 64 byte for cache and perfermance reasone
        /// @param sizeofs sizeof single entity
        /// @param count number of entities
        /// @return new array size
        inline static uint32_t getComponentArraySize(uint32_t sizeofs, uint32_t count){
            return (sizeofs*count+0x3F)&0xFFFFFFC0;
        }
        /// @brief calculate real aligned size of SOA
        /// @param sizeofs array of size of each component
        static uint32_t calculateSpaceRequirement(uint32_t entity_count,const span<uint16_t> sizeofs){
            int size = 0;
            for (uint16_t v:sizeofs)
                size += getComponentArraySize(v, entity_count);
            return size;
        }
        /// @brief finds suitable capacity
        static uint32_t calculateChunkCapacity(uint32_t chunk_size,const span<uint16_t> sizeofs){
            uint32_t total_size = 0;
            for (uint16_t v:sizeofs)
                total_size += v;
            if(total_size < 1)
                throw std::length_error("calculateChunkCapacity(): chunk total_size is zero");
            uint32_t capacity = chunk_size / total_size;
            while (calculateSpaceRequirement(capacity, sizeofs) > chunk_size)
                --capacity;
            return capacity;
        }
    };
} // namespace DOTS


#endif // ARCHETYPE_HPP
