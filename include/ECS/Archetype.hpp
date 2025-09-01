#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

#include <stdint.h>
#include <vector>
#include "defs.hpp"
#include "ArchetypeVersionManager.hpp"
#include "cutil/basics.hpp"
#include "cutil/mark_ptr.hpp"
#include "cutil/HashHelper.hpp"

class Test;

namespace ECS
{
    class Archetype;
}
namespace HashHelper
{
    inline uint32_t FNV1A32(const ECS::Archetype *ptr);
}

namespace ECS
{
    struct Chunk final {
        // MAGIC NUMBER, DO NOT TOUCH
        static constexpr uint32_t memoryOffset = 64; // (must be cache line aligned)
        // MAGIC NUMBER, DO NOT TOUCH
        static constexpr uint32_t memorySize = 16 * 1024; /// any number larger than 0xFFFF may cause overflow in offset array!
        static constexpr uint32_t bufferSize = memorySize - memoryOffset;
        // lower the number, the better component version-ing perform
        // MIN: 2 MAX: 0X4096 = (16*1024/4)
        static constexpr uint32_t maximumEntitiesPerChunk = 256;

        uint8_t memory[Chunk::memorySize];
    };
    class Archetype;



    /**
     * @brief A structure holding single archetype of components.
     * in a normal case type[0] must be Entity but this class has
     * nothing to do with data types it contain. at the end its
     * just a container.
     */
    class Archetype final
    {
    protected:

        friend class ChunkJobFunction;
        friend class EntityComponentManager;
        friend class ::Test;
        friend uint32_t HashHelper::FNV1A32(const ECS::Archetype*);

        // maximum number of entities that can be fit into a single chunk
        uint32_t chunkCapacity = 0;
        uint32_t lastChunkEntityCount = 0;
        // note: chunks are united, means
        // if we remove a entity for a random chunk,
        // last entity on last chunk will be
        // filled in it place so iterating more efficiently.
        std::vector<align_ptr<Chunk>> chunksData{};
        ArchetypeVersionManager chunksVersion{};


        // optimal for 16 component per archtype or less
        // Entity are istored as first type
        TypeID* __types = nullptr;
        // faster access to TypeID::realIndecies() for iteration
        uint16_t* __realIndecies = nullptr;
        uint32_t* __offsets = nullptr;
        uint16_t* __sizeOfs = nullptr;
        const uint32_t __type_count;
        // any index above/equal this is a tag and it size is equal to 0
        // firstTagIndex == 1 all tag, firstTagIndex == type_count no tag
        uint16_t firstTagIndex = 0;
        uint16_t flags=0;
        /// @brief archetype index in ECS archetype list, used for backward access.
        uint32_t archetypeIndex=0;
        Archetype(uint32_t typeCount):__type_count{typeCount} {};
    public:
        inline const_span<TypeID>   getType()   const {return {this->__types,  this->__type_count};}
        inline const_span<uint32_t> getOffset() const {return {this->__offsets,this->__type_count};}
        inline const_span<uint16_t> getSize()   const {return {this->__sizeOfs,this->__type_count};}
        inline const_span<uint16_t> getIndex()  const {return {this->__realIndecies,this->__type_count};}

        static mark_ptr<Archetype> createArchetype(const_span<TypeID> types);

        ~Archetype(/* args */) noexcept {
            auto archSizes {this->getSize().data()};
            const_span<align_ptr<ECS::Chunk>> archChunks {this->chunksData};
            auto archOffsets {this->getOffset().data()};
            auto archTypes {this->getType().data()};

            for (uint32_t typeIndex = 0; typeIndex < this->nonZeroSizedTypesCount(); typeIndex++)
                if(auto destructor =  getTypeInfo(archTypes[typeIndex]).destructor;destructor)
                    for (uint32_t chunkIndex = 0; chunkIndex < archChunks.size(); chunkIndex++)
                    {
                        uint8_t * const componentMemory =
                            archChunks[chunkIndex]->memory + archOffsets[typeIndex];
                        const uint32_t entityCount = (chunkIndex + 1) == archChunks.size() ?
                            this->lastChunkEntityCount : this->chunkCapacity;
                        const uint32_t typeSize = archSizes[typeIndex];
                        for (uint32_t valueIndex = 0; valueIndex < entityCount; valueIndex++)
                            destructor(componentMemory + typeSize*valueIndex);

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
        uint32_t capacity() const noexcept {
            return chunkCapacity * chunksVersion.capacity();
        }
        bool empty() const noexcept {
            return this->chunksVersion.empty();
        }
        // this fucntion is a little more expensive than empty
        uint32_t count() const noexcept {
            const uint32_t v = (uint32_t) this->chunksData.size();
            if(v < 1)
                return 0;
            else
                return (v-1) * chunkCapacity + lastChunkEntityCount;
        }

        inline uint16_t nonZeroSizedTypesCount() const noexcept {
            return firstTagIndex;
        }

        /// @brief find or create an index on chunks list.
        /// @return index within the archetype
        uint32_t createEntity(version_t globalVersion = 0);

        /// @brief remove an entity from archtype and returns entity value of the entity that must replaced it
        uint32_t removeEntity(uint32_t entityIndex);

        // requesting component that does not exist results undefined behaviour

        /// @brief returns memory address of a given component and entity
        /// @param componentIndex component index inside the archetype
        /// @param entityIndex entity index inside the archetype
        /// @param newVersion update version because of R/W access
        /// @return returns pointer or exception
        void* getComponent(uint16_t componentIndex,uint32_t entityIndex, version_t newVersion=0);
        const void* getComponent(uint16_t componentIndex,uint32_t entityIndex) const;

        void* getChunkComponent(uint16_t componentIndex,size_t chunkIndex){
            uint8_t * const mem = this->chunksData.at(chunkIndex)->memory;
            return mem + getOffset().at(componentIndex);
        }

        /// TODO: binary search?
        /// @note not flag sensitive
        /// @return -1 if not found
        int32_t getIndex(TypeID t) const noexcept;
    
        /// @brief locate every type index within archtype
        /// @note not flag sensitive
        /// @param t sorted array of types
        /// @param out output buffer pointer
        /// @return true on success on locating every type
        bool getIndecies(const_span<TypeID> t, uint16_t* out) const noexcept;

        /// @brief check if this archetype matches exact the same with param _types
        /// @note flag sensitive
        /// @param types sorted array of types
        /// @return true uf matches
        inline bool operator ==(const_span<TypeID> types) const noexcept {
            return this->getType() == types;
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
        static uint32_t calculateSpaceRequirement(uint32_t entity_count,const_span<uint16_t> sizeofs){
            uint32_t size = 0;
            for (const uint16_t v:sizeofs)
                size += (uint32_t) alignTo64(v, entity_count);
            return size;
        }
        /// @brief finds suitable capacity
        static uint32_t calculateChunkCapacity(uint32_t chunk_size,const_span<uint16_t> sizeofs){
            uint32_t total_size = 0;
            for (const uint16_t v:sizeofs)
                total_size += v;
            if(total_size < 1)
                throw std::length_error("calculateChunkCapacity(): chunk total_size is zero");
            uint32_t capacity = chunk_size / total_size;
            while (calculateSpaceRequirement(capacity, sizeofs) > chunk_size)
                --capacity;
            return capacity;
        }


    public:
        /// @note flag sensitive
        bool hasComponent(TypeID type) const;

        /// @note flag sensitive
        /// @param types sorted list of types,
        bool hasComponents(const_span<TypeID> types) const;

        /// @brief same as hasComponents without few optimizations for unordered type lists
        /// @note flag sensitive
        /// @param entity unsorted list of types you are looking for.
        bool hasComponentsSlow(const_span<TypeID> types) const;

    protected:
        /// @brief in-archetype delete operation, 
        /// @note dsr.~T(); pull src;  memcpy(dst,src);
        /// @param entity srcIndex to be removed
        /// @return value of entity that has replaced it, Entity::null if nothing happened
        Entity managedRemoveEntity(uint32_t dstIndex);

        /// @brief in-archetype move operation
        /// @note dst.~T(); memcpy(dst,src);
        /// @param entity value of srcIndex to be updated
        Entity moveComponentValues(uint32_t dstIndex, uint32_t srcIndex);

        /// @brief in-archetype move operation
        /// @note memcpy(dst,src);
        /// @param entity value of srcIndex to be updated
        Entity copyComponentValues(uint32_t dstIndex, uint32_t srcIndex);

        /// @brief between-archetype move operation
        /// @note dst.~T(); src.T(); memcpy(dst,src);
        /// @param entity value of srcIndex to be updated
        Entity moveComponentValues(Archetype &srcArchetype, uint32_t dstIndex, uint32_t srcIndex);

        // over chunk copy
        void inArchetypeCopy(void *__restrict__ srcChunk, void *__restrict__ dstChunk ,const uint32_t srcInChunkIndex, const uint32_t dstInChunkIndex);

        // in chunk copy
        void inArchetypeCopy(void *chunk, const uint32_t srcInChunkIndex, const uint32_t dstInChunkIndex);

        /// @brief call destructor function of components
        /// @note dst.~T();
        /// @param index index inside archetype
        void callComponentDestructor(uint32_t entityIndex);

        /// @brief call constructor function of components
        /// @note dst.T();
        /// @param index index inside archetype
        /// @param entity entity value to be storeds
        void callComponentConstructor(uint32_t entityIndex, Entity entity = Entity::null);
    };
} // namespace ECS

namespace HashHelper
{
    inline uint32_t FNV1A32(const ECS::Archetype *ptr){
        return HashHelper::FNV1A32(ptr->getType());
    }
}

#endif // ARCHETYPE_HPP
