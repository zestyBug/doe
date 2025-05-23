#if !defined(CHUNKLISTMAP_HPP)
#define CHUNKLISTMAP_HPP

#include "basics.hpp"
#include "defs.hpp"
#include "cutil/hash128.hpp"
#include "cutil/span.hpp"
#include <vector>

namespace DOTS
{
    class Archetype;

    class ArchetypeListMap
    {
        static const uint32_t _AValidHashCode = 0x00000001;
        static const uint32_t _SkipCode = 0xFFFFFFFF;
        /// @brief generates hash code for a archetype
        /// @param types note that type flags are also included, like disable-ness, prefab-being
        /// @return (may-modified) hash value
        static uint32_t getHashCode(span<TypeIndex> types)
        {
            uint32_t result = hash128::FNV1A32(types);
            if (result == 0 || result == _SkipCode)
                result = _AValidHashCode;
            return result;
        }
    public:

        ArchetypeListMap() = default;
        ArchetypeListMap(const ArchetypeListMap&) = delete;
        ArchetypeListMap(ArchetypeListMap&& v){
            *this = std::move(v);
        }
        ArchetypeListMap& operator=(const ArchetypeListMap&) = delete;
        ArchetypeListMap& operator=(ArchetypeListMap&& v){
            if(this != &v){
                this->emptyNodes = v.emptyNodes;
                this->skipNodes = v.skipNodes;
                v.emptyNodes=0;
                v.skipNodes=0;
                this->hashes     = std::move(v.hashes);
                this->archetypes = std::move(v.archetypes);
            }
            return *this;
        }
        inline uint32_t size() const {
            return hashes.size();
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
            return 64 / sizeof(uint32_t);
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


        void setCapacity(uint32_t capacity);
        void init(uint32_t count);
        void appendFrom(ArchetypeListMap& src);
        /// @brief find archtype using hash list
        /// @return archtype or nullptr
        Archetype* tryGet(span<TypeIndex> types) const;
        // tryGet + exception if not found
        Archetype* get(span<TypeIndex> types) const;
        void resize(uint32_t size);
        void add(Archetype* archetype);
        int  indexOf(Archetype* archetype) const;
        void remove(Archetype* archetype);
        bool contains(Archetype* archetype) const;
        // the whole popuse is to free space when object is unused but still in memory
        void dispose(){
            hashes= std::vector<uint32_t>();
            archetypes = std::vector<Archetype*>();
            emptyNodes=0;
            skipNodes=0;
        }
    protected:
        std::vector<uint32_t> hashes{};
        std::vector<Archetype*> archetypes{};
        uint32_t emptyNodes=0;
        uint32_t skipNodes=0;
    };

} // namespace DOT


#endif // CHUNKLISTMAP_HPP
