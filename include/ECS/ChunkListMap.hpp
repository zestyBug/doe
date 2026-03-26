#if !defined(CHUNKLISTMAP_HPP)
#define CHUNKLISTMAP_HPP

#include "cutil/basics.hpp"
#include "cutil/HashHelper.hpp"
#include "Base/Chunk.hpp"
#include "SharedComponent.hpp"
#include <vector>

namespace ECS
{
    class Archtype;
    /**
     * @brief A map to find chunks with free some slots and a given specific shared component values.
     */
    class ChunkListMap final
    {
    protected:
        static const uint32_t _AValidHashCode = 0x00000001;
        /// @brief When current hash value is invalid but you should not stop searching.
        static const uint32_t _SkipCode = 0xFFFFFFFF;

        align_ptr<uint32_t[]> hashes{};
        align_ptr<Chunk*  []> chunks{};
        uint32_t _size = 0;
        uint32_t _capacity = 0;
        Archetype* archetype;
        uint32_t emptyNodes=0;
        uint32_t skipNodes=0;

        void resize(uint32_t size){
            if (size < minimumSize())
                size = minimumSize();
            if (size == this->size())
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
            if (sharedComponents.stride == sizeof(void*))
            {
                result = HashHelper::FNV1A32(sharedComponents.firstIndex, numSharedComponents * sizeof(void*));
            }
            else
            {
                void* indexArray[numSharedComponents];
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
            this->emptyNodes = v.emptyNodes;
            this->skipNodes = v.skipNodes;
            v.emptyNodes=0;
            v.skipNodes=0;
        }
        ChunkListMap& operator=(const ChunkListMap&) = delete;
        ChunkListMap& operator=(ChunkListMap&& v){
            if(this != &v){
                this->hashes     = std::move(v.hashes);
                this->chunks     = std::move(v.chunks);
                this->emptyNodes = v.emptyNodes;
                this->skipNodes  = v.skipNodes;
                v.emptyNodes=0;
                v.skipNodes=0;
            }
            return *this;
        }
        inline uint32_t size() const {
            return _size;
        }
        inline uint32_t unoccupiedNodes() const {
            return emptyNodes + skipNodes;
        }
        inline uint32_t occupiedNodes() const {
            return size() - unoccupiedNodes();
        }
        inline bool isEmpty() const {
            return occupiedNodes() == 0;
        }
        /// @brief ! suppose size is power of 2
        inline uint32_t hashMask() const {
            return size() - 1;
        }
        inline uint32_t minimumSize() const {
            return 64 / sizeof(Chunk*);
        }
        inline void possiblyGrow()
        {
            if (unoccupiedNodes() < size() / 3)
                resize(size() * 2);
        }
        inline void possiblyShrink()
        {
            if (occupiedNodes() < size() / 3)
                resize(size() / 2);
        }
        void init(Archetype* _archetype, uint32_t count = 0){
            this->archetype = _archetype;
            if (count < minimumSize())
                count = minimumSize();

            // is power of 2?
            if(0 != (count & (count - 1)))
                throw std::invalid_argument("Init(): count must be power of 2");

            hashes = make_align<uint32_t[]>(count);
            memset(hashes.get(),0,count * sizeof(uint32_t));
            chunks = make_align<Chunk*[]>(count);

            emptyNodes = count;
            skipNodes = 0;
        }
        void appendFrom(ChunkListMap& src){
            for (uint32_t offset = 0; offset < src.size(); ++offset)
            {
                uint32_t hash = src.hashes[offset];
                if (hash != 0 && hash != _SkipCode)
                    add(src.chunks[offset]);
            }
        }
        /**
         * @note increasing capacity may ruin the hash offsets, move the data to a bigger map instead.
         */
        void setCapacity(uint32_t newCapacity) {
            if (newCapacity < minimumSize())
                newCapacity = minimumSize();
            {
                align_ptr<uint32_t []> buffer = make_align<uint32_t []>(newCapacity);
                memset(buffer.get(),0,newCapacity * sizeof(uint32_t));
                memcpy(buffer.get(), hashes.get(),_capacity * sizeof(uint32_t));
                hashes = std::move(buffer);
            }
            {
                align_ptr<Chunk*[]> buffer = make_align<Chunk*[]>(newCapacity);
                memcpy(buffer.get(), hashes.get(),_capacity * sizeof(Chunk*));
                chunks = std::move(buffer);
            }
            _capacity = newCapacity;
        }
        // the whole popuse is to free space when object is unused but still in memory
        void reset(){
            hashes.reset();
            chunks.reset();
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