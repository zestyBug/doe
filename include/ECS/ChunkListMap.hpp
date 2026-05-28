#if !defined(CHUNKLISTMAP_HPP)
#define CHUNKLISTMAP_HPP

#include "cutil/basics.hpp"
#include "cutil/HashHelper.hpp"
#include "Base/Chunk.hpp"
#include "Base/SharedComponent.hpp"

namespace ECS
{
    class Archetype;
    /**
     * @brief A map to find chunks with free some slots and a given specific shared component values.
     */
    class ChunkListMap final
    {
    protected:
        static constexpr uint32_t _AValidHashCode = 0x00000001;
        /// @brief When current hash value is invalid but you should not stop searching.
        static constexpr uint32_t _SkipCode = 0xFFFFFFFF;

        std::unique_ptr<uint32_t[]> hashes{};
        Chunk**chunks = nullptr;
        Archetype* archetype;
        uint32_t _capacity = 0;
        uint32_t emptyNodes=0;

        void resize(uint32_t size);
        /// @brief generates hash code for a pointer
        /// @param sharedComponents note that type flags are also included, like disable-ness, prefab-being
        /// @return (may-modified) hash value
        static uint32_t getHashCode(const SharedComponentValues sharedComponents, uint32_t numSharedComponents);

    public:

        ChunkListMap() = default;
        ChunkListMap(const ChunkListMap&) = delete;
        ChunkListMap(ChunkListMap&& v)
            :hashes{std::move(v.hashes)},chunks{std::move(v.chunks)}
        {
            this->archetype = v.archetype;
            this->_capacity = v._capacity;
            this->emptyNodes = v.emptyNodes;
            v._capacity=0;
            v.emptyNodes=0;
        }
        ChunkListMap& operator=(const ChunkListMap&) = delete;
        ChunkListMap& operator=(ChunkListMap&& v){
            if(this != &v){
                this->hashes     = std::move(v.hashes);
                this->chunks     = std::move(v.chunks);
                this->archetype = v.archetype;
                this->_capacity = v._capacity;
                this->emptyNodes = v.emptyNodes;
                v._capacity=0;
                v.emptyNodes=0;
            }
            return *this;
        }
        uint32_t capacity() const {
            return _capacity;
        }
        inline uint32_t unoccupiedNodes() const {
            return emptyNodes;
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
        void init(Archetype* _archetype, uint32_t count = 0);
        void appendFrom(ChunkListMap& src);
        // the whole popuse is to free space when object is unused but still in memory
        void reset();
        void add(Chunk*);
        void remove(Chunk*);
        /// @brief find a pointer with a key using hash list
        /// @return value or nullptr
        Chunk* tryGet(const SharedComponentValues sharedComponentValues, uint32_t numSharedComponents) const;
        bool contains(Chunk*) const;
    };
}
#endif