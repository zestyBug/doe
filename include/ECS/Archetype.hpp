#include <stdint.h>
#include <vector>
#include <array>
#include <utility>
#include "defs.hpp"
#include "ArchetypeChunkData.hpp"
#include "basics.hpp"
#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

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

        friend class Register;
        // invalid index and max entity number
        static constexpr uint32_t INVALID_INDEX = 0xffffff;
        // maximum number of entities that can be fit into a single chunk
        size_t chunkCapacity = 0;
        size_t lastChunkEntityCount = 0;
        // note: chunks are united, means
        // if we remove a entity for a random chunk,
        // last entity on last chunk will be
        // filled in it place so iterating more efficiently.
        std::vector<Chunk> chunksData;
        ArchetypeChunkData chunks;
        // optimal for 16 component per archtype or less
        // Entity are istored as first type
        std::vector<TypeIndex> types;
        // faster access to TypeIndex::realIndex() for iteration
        std::vector<uint16_t> realIndex;
        std::vector<uint32_t> offsets;
        std::vector<uint16_t> sizeOfs;
        // any index above/equal this is a tag and it size is equal to 0
        // firstTagIndex == 1 all tag, firstTagIndex == types.size() no tag
        uint16_t firstTagIndex = 0;
        // index of Entity inside types and other arrays
        uint16_t entitiesIndex = 0;
        uint16_t flags;
        Archetype(/* args */) = default;
    public:
        Archetype(const Archetype&) = delete;
        Archetype(Archetype&&) = default;

        // estimated capacity
        // check before accessing an unallocated archtype
        size_t capacity() const {
            return chunkCapacity * chunks.capacity();
        }
        bool empty() const {
            return this->chunks.empty();
        }
        // this fucntion is a little more expensive than empty
        bool count() const {
            uint32_t v = this->chunks.count();
            if(v < 1)
                return 0;
            else
                return --v * chunkCapacity + lastChunkEntityCount;
        }

        static Archetype* createArchetype(const std::vector<TypeIndex>& types) {
            if(types.size() < 1 || types.size() > 0xFFF)
                throw std::invalid_argument("createArchetype(): archetype with unexpected component count");

            Archetype *arch = allocator<Archetype>().allocate(1);

            new (arch) Archetype();
            arch->types.resize(types.size()+1);
            arch->realIndex.resize(arch->types.size());

            TypeIndex ev = getTypeInfo<Entity>().value;

            uint32_t i = 0;
            for (;i < types.size(); ++i) {
                if(ev < types[i])
                    break;
                arch->types[i]=types[i];
                arch->realIndex[i] = types[i].realIndex();
            }
            arch->types[i] = ev;
            arch->entitiesIndex = i;
            i++;
            for (;i < types.size(); ++i)
                arch->types[i+1]=types[i];

            
            arch->chunks.initialize(arch->types.size());
            arch->offsets.resize(arch->types.size());
            arch->sizeOfs.resize(arch->types.size());
            

            {
                uint16_t i = arch->types.size();
                do arch->firstTagIndex = i;
                while (i!=0 && getTypeInfo(arch->types[--i]).size == 0);
                i++;
            }
            for (uint32_t i = 0; i < arch->types.size(); ++i)
            {
                if (i < arch->firstTagIndex){
                    const auto& cType = getTypeInfo(arch->types[i]);
                    arch->sizeOfs[i] = cType.size;
                }else
                    arch->sizeOfs[i] = 0;
            }
            for (TypeIndex type: arch->types)
            {
                if(type.isPrefab())
                    arch->flags |= TypeIndex::prefab;
            }

            arch->chunkCapacity = calculateChunkCapacity(Chunk::_BufferSize,arch->sizeOfs);

            if(arch->chunkCapacity < 4)
                throw std::length_error("createArchetype(): LARGE COMPONENTS! X(");
            
            uint32_t usedBytes = Chunk::_BufferOffset;
            for (uint32_t typeIndex = 0; typeIndex < arch->offsets.size(); typeIndex++)
            {
                uint32_t sizeOf = arch->sizeOfs[typeIndex];

                // align usedBytes upwards (eating into alignExtraSpace) so that
                // this component actually starts at its required alignment.
                // Assumption is that the start of the entire data segment is at the
                // maximum possible alignment.
                arch->offsets[typeIndex] = usedBytes;
                usedBytes += getComponentArraySize(sizeOf, arch->chunkCapacity);
            }

            return arch;
        }
        
        uint32_t createEntity(Entity e) {
            uint32_t res = 0;
            uint8_t* lastChunkMem = nullptr;

            if(this->chunksData.size() == 0 || lastChunkEntityCount == this->chunkCapacity){
                lastChunkEntityCount=0;
                Chunk &chunk = this->chunksData.emplace_back();
                chunk.memory = allocator().allocate(Chunk::_ChunkSize);
                lastChunkMem = (uint8_t*)chunk.memory;
                chunks.add(0,0);
            }else
                lastChunkMem = (uint8_t*)this->chunksData.back().memory;
            res = this->count();
            if(res >= INVALID_INDEX)
                throw std::out_of_range("entity count limit reached");
            //
            ((Entity*)(lastChunkMem+offsets[entitiesIndex]))[getInChunkIndex(res)] = e;
            lastChunkEntityCount++;
            return res;
        }

        // remove an entity from archtype and returns entity value of 
        // the entity that has replaced it
        Entity removeEntity(uint32_t entityIndex, bool destruction = true){
            if(this->chunksData.size() < 1)
                throw std::runtime_error("removeEntity(): empty archetype");
            if(lastChunkEntityCount < 1)
                throw std::runtime_error("removeEntity(): unexpected internal error");

             
                
            
                const uint32_t chunkIndex = this->getChunkIndex(entityIndex);
                const uint32_t inChunkIndex = this->getInChunkIndex(entityIndex);
                // im not sure that it wont break, so a at() will do the job
                uint8_t * const chunkMemory = (uint8_t*) this->chunksData.at(chunkIndex).memory;

                const uint32_t lastChunkIndex = this->chunksData.size() - 1;
                const uint32_t inLastChunkIndex = lastChunkEntityCount - 1;
                uint8_t const * const lastChunkMemory = (uint8_t*) this->chunksData.at(lastChunkIndex).memory;

                if(chunkIndex >= lastChunkIndex)
                    if(inChunkIndex > inLastChunkIndex)
                        throw std::invalid_argument("removeEntity(): entity does not exists");
            
            Entity lastEntity = ((Entity*)(chunkMemory+offsets[entitiesIndex]))[inChunkIndex];
            
            if(destruction)
                for(size_t i=0;i < this->types.size();i++) {
                    const auto& info = getTypeInfo(types[i]);
                    if(info.destructor){
                        uint8_t * const ptr1 = chunkMemory + offsets[i] + (sizeOfs[i] * inChunkIndex);
                        info.destructor(ptr1);
                    }
                }

            // last entity in archetype

            
            // if last entity is the deleted entity
            if(chunkIndex >= lastChunkIndex)
                if(inChunkIndex == inLastChunkIndex)
                    goto END;



            for(size_t i=0;i < this->types.size();i++) {
                const auto&    info = getTypeInfo(types[i]);
                      uint8_t *ptr1 = chunkMemory + offsets[i] + (sizeOfs[i] * inChunkIndex);
                const uint8_t *ptr2 = lastChunkMemory + offsets[i] + (sizeOfs[i] * inLastChunkIndex);
                memcpy(ptr1,ptr2,info.size);
            }

            END:

            lastChunkEntityCount--;

            // 0 means last element on the chunk is either moved to
            // the deleted entity position or is the deleted entity itself
            // so chunk must be cleared
            if(lastChunkEntityCount == 0){
                this->chunksData.pop_back();
                this->chunks.popBack();
                if(lastChunkIndex != 0)
                    lastChunkEntityCount = this->chunkCapacity;
            }
            return lastEntity;
        }
        
        // requesting component that does not exist results undefined behaviour
        
        /// @brief returns memory address of a given component and entity
        /// @param componentIndex component inside the archetype index
        /// @param entityIndex entity inside the archetype index
        /// @return returns pointer or exception
        void* getComponent(uint16_t componentIndex,uint32_t entityIndex){
            uint32_t inChunkIndex = getInChunkIndex(entityIndex);
            uint32_t chunkIndex = getChunkIndex(entityIndex);
            uint8_t *chunkMemory = (uint8_t *)chunksData.at(chunkIndex).memory;
            if((chunkIndex+1) >= chunksData.size())
                if(inChunkIndex >= lastChunkEntityCount)
                    throw std::out_of_range("getComponent(): invalid entityIndex");
            return chunkMemory + offsets.at(componentIndex) + (sizeOfs.at(componentIndex) * inChunkIndex);
        }
        // requesting component that does not exist results undefined behaviour
        template<typename T>
        T* getComponent(uint32_t entityIndex){
            int32_t componentIndex = getIndex<T>();
            if(componentIndex < 0)
                throw std::invalid_argument("getComponent<T>(): invalid Type");
            return (T*)getComponent(componentIndex,entityIndex);
        }
        // requesting component that does not exist results undefined behaviour
        template<typename T>
        span<T> getChunkComponent(size_t chunkIndex){
            int32_t componentIndex = getIndex<T>();
            if(componentIndex < 0)
                throw std::invalid_argument("getComponent<T>(): invalid Type");
            uint8_t * const mem = (uint8_t*) this->chunksData.at(chunkIndex).memory;
            if((1+chunkIndex) >= this->chunksData.size())
                return span<T>( (T*)(mem + offsets.at(componentIndex)) , lastChunkEntityCount );
            else 
                return span<T>( (T*)(mem + offsets.at(componentIndex)) , chunkCapacity );
        }
        void* getChunkComponent(uint16_t componentIndex,size_t chunkIndex){
            uint8_t * const mem = (uint8_t*) this->chunksData.at(chunkIndex).memory;
            return mem + offsets.at(componentIndex);
        }

        /// @brief retriev entity id of given index
        /// @param entityIndex accepts unsafe indecies
        /// @return Entity::null on failure
        Entity getEntity(uint32_t entityIndex) const {
            
            uint32_t lastChunkIndex = chunksData.size();

            if(lastChunkIndex < 1)
                return Entity::null;
            lastChunkIndex--;

            const uint32_t chunkIndex = getChunkIndex(entityIndex);

            if(lastChunkIndex <= chunkIndex)
                return Entity::null;

            const uint32_t inChunkIndex = getInChunkIndex(entityIndex);
            const uint32_t inLastChunkIndex = lastChunkEntityCount - 1;

            if(chunkIndex >= lastChunkIndex)
                if(inChunkIndex > inLastChunkIndex)
                    return Entity::null;

            uint8_t * const chunkMemory = (uint8_t*) chunksData.at(chunkIndex).memory;

            
            return ((Entity*)(chunkMemory+offsets[this->entitiesIndex]))[inChunkIndex];
        }
        Entity& getEntityUnsafe(uint32_t entityIndex) {
            const uint32_t chunkIndex = getChunkIndex(entityIndex);
            const uint32_t inChunkIndex = getInChunkIndex(entityIndex);
            uint8_t * const chunkMemory = (uint8_t*) chunksData.at(chunkIndex).memory;
            return ((Entity*)(chunkMemory+offsets[this->entitiesIndex]))[inChunkIndex];
        }
        // TODO: binary search?
        
        /// @brief simple linear search to find index in archtype
        /// @tparam T Type
        /// @return index or -1 if archtype doesnt contain type T
        template<typename T>
        int32_t getIndex(){
            uint16_t rIndex = getTypeInfo<T>().value.realIndex();
            for (size_t i = 0; i < realIndex.size(); i++)
                if(realIndex[i] == rIndex)
                    return i;
            return -1;
        }
        int32_t getIndex(TypeIndex t){
            uint16_t rIndex = t.realIndex();
            for (size_t i = 0; i < realIndex.size(); i++)
                if(realIndex[i] == rIndex)
                    return i;
            return -1;
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
