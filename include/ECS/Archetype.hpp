#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

#include <stdint.h>
#include <vector>
#include <array>
#include <utility>
#include "defs.hpp"
#include "ArchetypeVersionManager.hpp"
#include "cutil/basics.hpp"
#include "cutil/unique_ptr.hpp"

void Test();

namespace ECS
{
    struct Chunk final {
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
        }

        static constexpr uint32_t memoryOffset = 64; // (must be cache line aligned)
        static constexpr uint32_t memorySize = 16 * 1024; /// any number larger than 0xFFFF may cause overflow in offset array!
        static constexpr uint32_t bufferSize = memorySize - memoryOffset;
        // lower the number, the better component version-ing perform
        static constexpr uint32_t maximumEntitiesPerChunk = 512;
    };
    class Archetype;
    
    using ArchetypeHolder = unique_ptr<Archetype,allocator<Archetype>>;

    /**
     * @brief A structure holding single archetype of components.
     * in a normal case type[0] must be Entity but this class has
     * nothing to do with data types it contain. at the end its
     * just a container.
     */
    class Archetype final
    {
    protected:

        friend class ThreadPool;
        friend class EntityComponentManager;
        friend void ::Test();
        // maximum number of entities that can be fit into a single chunk
        uint32_t chunkCapacity = 0;
        uint32_t lastChunkEntityCount = 0;
        // note: chunks are united, means
        // if we remove a entity for a random chunk,
        // last entity on last chunk will be
        // filled in it place so iterating more efficiently.
        std::vector<Chunk,allocator<Chunk>> chunksData{};
        ArchetypeVersionManager chunksVersion{};


        // optimal for 16 component per archtype or less
        // Entity are istored as first type
        span<TypeID> types{};
        // faster access to TypeID::realIndecies() for iteration
        span<uint16_t> realIndecies{};
        span<uint32_t> offsets{};
        span<uint16_t> sizeOfs{};
        // any index above/equal this is a tag and it size is equal to 0
        // firstTagIndex == 1 all tag, firstTagIndex == types.size() no tag
        uint16_t firstTagIndex = 0;
        uint16_t flags=0;
        /// @brief archetype index in ECS archetype list, used for backward access.
        uint32_t archetypeIndex=0;
        Archetype(/* args */) = default;
    public:

        static ArchetypeHolder createArchetype(span<TypeID> types);

        ~Archetype(/* args */) {
            span<uint16_t> archSizes = this->sizeOfs;
            span<Chunk> archChunks = this->chunksData;
            span<uint32_t> archOffsets = this->offsets;
            span<TypeID> archTypes = this->types;
            for (uint32_t typeIndex = 0; typeIndex < this->nonZeroSizedTypesCount(); typeIndex++)
            {
                auto destructor =  getTypeInfo(archTypes[typeIndex]).destructor;
                for (uint32_t chunkIndex = 0; chunkIndex < archChunks.size(); chunkIndex++)
                {
                    uint8_t * const componentMemory =
                        (uint8_t*)(archChunks[chunkIndex].memory) + archOffsets[typeIndex];
                    const uint32_t entityCount = (chunkIndex + 1) == archChunks.size() ?
                        this->lastChunkEntityCount : this->chunkCapacity;
                    const uint32_t typeSize = archSizes[typeIndex];
                    for (uint32_t valueIndex = 0; valueIndex < entityCount; valueIndex++)
                        destructor(componentMemory + typeSize*valueIndex);

                }
            }
        }

        /// @brief maximum number of entities an archetype can hold any value equal higther that this can be used as invalid value
        static constexpr uint32_t MaxEntityIndex =  INT32_MAX;
        static constexpr uint32_t NullEntityIndex =0x80000000;
        /// @brief maximum number of chunks an archetype can hold any value equal higther that this can be used as invalid value
        static constexpr uint32_t MaxChunkIndex =  INT32_MAX;
        static constexpr uint32_t NullChunkIndex =0x80000000;
        /// @brief maximum number of type an archetype can manage, any number higher than this may lead to overflow
        static constexpr uint32_t MaxTypePerArchetype = 0xff;


        Archetype& operator =(const Archetype&) = delete;
        Archetype(const Archetype&) = delete;
        // data structor stores pointer to itself
        // any move operation requires recalculation
        Archetype(Archetype&&) = delete;

        // estimated capacity
        // check before accessing an unallocated archtype
        uint32_t capacity() const {
            return chunkCapacity * chunksVersion.capacity();
        }
        bool empty() const {
            return this->chunksVersion.empty();
        }
        // this fucntion is a little more expensive than empty
        uint32_t count() const {
            const uint32_t v = (uint32_t) this->chunksData.size();
            if(v < 1)
                return 0;
            else
                return (v-1) * chunkCapacity + lastChunkEntityCount;
        }

        inline uint16_t nonZeroSizedTypesCount() const {
            return firstTagIndex;
        }

        /// @brief find or create an index on chunks list.
        /// @return index within the archetype
        uint32_t createEntity(version_t globalVersion = 0);

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

        /// TODO: binary search?
        /// @note not flag sensitive
        /// @return -1 if not found
        int32_t getIndex(TypeID t) const;
    
        /// @brief locate every type index within archtype
        /// @note not flag sensitive
        /// @param t sorted array of types
        /// @param out output buffer pointer
        /// @return true on success on locating every type
        bool getIndecies(span<TypeID> t, uint16_t* out) const;

        uint32_t getHash() const;
        /// @brief check if this archetype matches exact the same with param _types
        /// @note flag sensitive
        /// @param _types sorted array of types
        /// @return true uf matches
        inline bool operator ==(span<TypeID> _types) const {
            return (this->types) == _types;
        }
    protected:
        // the entity chunk index
        inline uint32_t getChunkIndex(const uint32_t i) const {
            return (i / this->chunkCapacity);
        }
        // index within a chunk
        inline uint32_t getInChunkIndex(const uint32_t i) const {
            return (i % this->chunkCapacity);
        }
        /// @brief calculate real aligned size of SOA
        /// @param sizeofs array of size of each component
        static uint32_t calculateSpaceRequirement(uint32_t entity_count,const span<uint16_t> sizeofs){
            uint32_t size = 0;
            for (uint16_t v:sizeofs)
                size += (uint32_t) alignTo64(v, entity_count);
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



        /// @note flag sensitive
        bool hasComponent(TypeID type) const;

        /// @note flag sensitive
        /// @param types sorted list of types,
        bool hasComponents(span<TypeID> types) const;

        /// @brief same as hasComponents without few optimizations for unordered type lists
        /// @note flag sensitive
        /// @param entity unsorted list of types you are looking for.
        bool hasComponentsSlow(span<TypeID> types) const;


        /// @brief in-archetype delete operation, 
        /// @note dsr.~T(); pull src;  memcpy(dst,src);
        /// @param entity srcIndex to be removed
        /// @return value of entity that has replaced it, Entity::null if nothing happened
        static Entity managedRemoveEntity(Archetype *archetype, uint32_t dstIndex);

        /// @brief in-archetype move operation
        /// @note dst.~T(); memcpy(dst,src);
        /// @param entity value of srcIndex to be updated
        static Entity moveComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex);

        /// @brief in-archetype move operation
        /// @note memcpy(dst,src);
        /// @param entity value of srcIndex to be updated
        static Entity copyComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex);

        /// @brief between-archetype move operation
        /// @note dst.~T(); src.T(); memcpy(dst,src);
        /// @param entity value of srcIndex to be updated
        static Entity moveComponentValues(Archetype *dstArchetype, uint32_t dstIndex,Archetype *srcArchetype, uint32_t srcIndex);

        void inArchetypeCopy(void * const srcChunk, void * const dstChunk ,const uint32_t srcInChunkIndex, const uint32_t dstInChunkIndex);

        /// @brief call destructor function of components
        /// @note dst.~T();
        /// @param index index inside archetype
        void callComponentDestructor(uint32_t entityIndex);

        /// @brief call constructor function of components
        /// @note dst.T();
        /// @param index index inside archetype
        /// @param entity entity value to be storeds
        void callComponentConstructor(uint32_t entityIndex, Entity e = Entity::null);
    };
} // namespace ECS


#endif // ARCHETYPE_HPP
