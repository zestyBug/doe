#if !defined(CHUNKLISTMAP_HPP)
#define CHUNKLISTMAP_HPP

#include "cutil/basics.hpp"
#include "cutil/HashHelper.hpp"
#include "Base/Chunk.hpp"
#include "Base/SharedComponent.hpp"

namespace ECS
{
    class Archtype;
    /**
     * @brief A map to find chunks with free some slots and a given specific shared component values.
     */
    class ChunkListMap final
    {
    protected:
        static constexpr uint32_t _AValidHashCode = 0x00000001;
        /// @brief When current hash value is invalid but you should not stop searching.
        static constexpr uint32_t _SkipCode = 0xFFFFFFFF;

        align_ptr<uint32_t[]> hashes{};
        Chunk**chunks = nullptr;
        Archetype* archetype;
        uint32_t _capacity = 0;
        uint32_t emptyNodes=0;
        uint32_t skipNodes=0;

        void resize(uint32_t size){
            if (size < minimumSize())
                size = minimumSize();
            if (size == this->capacity())
                return;
            ChunkListMap temp;
            temp.init(this->archetype, size);
            temp.appendFrom(*this);
            *this = std::move(temp);
        }
        /// @brief generates hash code for a pointer
        /// @param sharedComponents note that type flags are also included, like disable-ness, prefab-being
        /// @return (may-modified) hash value
        static uint32_t getHashCode(const SharedComponentValues sharedComponents, uint32_t numSharedComponents)
        {
            uint32_t result;
            if (sharedComponents.stride == sizeof(SharedComponentIndex))
            {
                result = HashHelper::FNV1A32(sharedComponents.firstIndex, numSharedComponents * sizeof(void*));
            }
            else
            {
                SharedComponentIndex indexArray[numSharedComponents];
                for (uint32_t i = 0; i < numSharedComponents; ++i)
                    indexArray[i] = sharedComponents[i];
                result = HashHelper::FNV1A32(indexArray, numSharedComponents * sizeof(uint32_t));
            }
            if (result == 0 || result == _SkipCode)
                result = _AValidHashCode;
            return result;
        }

    public:

        ChunkListMap() = default;
        ChunkListMap(const ChunkListMap&) = delete;
        ChunkListMap(ChunkListMap&& v)
            :hashes{std::move(v.hashes)},chunks{std::move(v.chunks)}
        {
            this->archetype = v.archetype;
            this->_capacity = v._capacity;
            this->emptyNodes = v.emptyNodes;
            this->skipNodes = v.skipNodes;
            v._capacity=0;
            v.emptyNodes=0;
            v.skipNodes=0;
        }
        ChunkListMap& operator=(const ChunkListMap&) = delete;
        ChunkListMap& operator=(ChunkListMap&& v){
            if(this != &v){
                this->hashes     = std::move(v.hashes);
                this->chunks     = std::move(v.chunks);
                this->archetype = v.archetype;
                this->_capacity = v._capacity;
                this->emptyNodes = v.emptyNodes;
                this->skipNodes  = v.skipNodes;
                v._capacity=0;
                v.emptyNodes=0;
                v.skipNodes=0;
            }
            return *this;
        }
        uint32_t capacity() const {
            return _capacity;
        }
        inline uint32_t unoccupiedNodes() const {
            return emptyNodes + skipNodes;
        }
        inline uint32_t occupiedNodes() const {
            return capacity() - unoccupiedNodes();
        }
        inline bool isEmpty() const {
            return occupiedNodes() == 0;
        }
        /// @brief ! suppose size is power of 2
        inline uint32_t hashMask() const {
            return capacity() - 1;
        }
        inline uint32_t minimumSize() const {
            return 64 / sizeof(Chunk*);
        }
        inline void possiblyGrow()
        {
            if (unoccupiedNodes() < capacity() / 3)
                resize(capacity() * 2);
        }
        inline void possiblyShrink()
        {
            if (occupiedNodes() < capacity() / 3)
                resize(capacity() / 2);
        }
        void init(Archetype* _archetype, uint32_t count = 0){
            if (count < minimumSize())
                count = minimumSize();

            // is power of 2?
            if(0 != (count & (count - 1)))
                throw std::invalid_argument("init(): count must be power of 2");
            // listWithEmptySlotsIndex is a signed int
            if(count > INT32_MAX)
                throw std::invalid_argument("init(): too large map");
            const uint32_t size1 = alignTo64(sizeof(uint32_t),count);
            const uint32_t size2 = alignTo64(sizeof(Chunk*),count);
            uint8_t* ptr = allocator<>().allocate(size1+size2);
            hashes.reset((uint32_t*)ptr);
            memset(hashes.get(),0,size1);
            chunks = (Chunk**)(ptr + size1);
            this->archetype = _archetype;
            _capacity = count;
            emptyNodes = count;
            skipNodes = 0;
        }
        void appendFrom(ChunkListMap& src) {
            for (uint32_t offset = 0; offset < src.capacity(); ++offset)
            {
                uint32_t hash = src.hashes[offset];
                if (hash != 0 && hash != _SkipCode)
                    add(src.chunks[offset]);
            }
        }
        // the whole popuse is to free space when object is unused but still in memory
        void reset(){
            hashes.reset();
            chunks = nullptr;
            emptyNodes=0;
            skipNodes=0;
        }
        void add(Chunk*);
        void remove(Chunk*);
        /// @brief find a pointer with a key using hash list
        /// @return value or nullptr
        Chunk* tryGet(const SharedComponentValues sharedComponentValues, uint32_t numSharedComponents) const;
        bool contains(Chunk*) const;
    };
}
#endif