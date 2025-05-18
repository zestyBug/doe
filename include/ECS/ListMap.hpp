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

    class ListMap
    {
        static const uint32_t _AValidHashCode = 0x00000001;
        static const uint32_t _SkipCode = 0xFFFFFFFF;
    public:
        using Key = uint32_t;
        using Value = uint32_t;
        
        static const Value invalideValue = 0xffffff;


        ListMap() = default;
        ListMap(const ListMap&) = delete;
        ListMap(ListMap&& v){
            *this = std::move(v);
        }
        ListMap& operator=(const ListMap&) = delete;
        ListMap& operator=(ListMap&& v){
            if(this != &v){
                this->emptyNodes = v.emptyNodes;
                this->skipNodes = v.skipNodes;
                v.emptyNodes=0;
                v.skipNodes=0;
                this->hashes = std::move(v.hashes);
                this->value = std::move(v.value);
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

        void init(int count);
        
        //uint32_t TryGet(SharedComponentValues sharedComponentValues, int numSharedComponents);
        
        
        
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
        /// @brief add new entity to list, may throw an exception
        /// @param hash hash of data
        /// @param value a unique value, it is kept and can be retrieved by index, or by searching
        /// @return returns index of value in array, keep it, index maybe invalidated on removing other entities!
        uint32_t add(uint32_t hash,Value v);
        void remove(int32_t index);
        void removeByHash(uint32_t hash);
        // 
        bool contains(int32_t index);
        inline bool containsByHash(uint32_t hash){
            return indexOf(hash) != -1;
        }
        // retrieve index by value or -1
        int32_t indexOf(uint32_t hash);
        // the whole promise of it is that hash makes it faster
        Value tryGetValue(uint32_t hash);
        void appendFrom(const ListMap& src);
        void resize(int size);
    protected:
        
        std::vector<uint32_t> hashes;
        std::vector<Value> value;

        uint32_t emptyNodes=0;
        uint32_t skipNodes=0;
    };

} // namespace DOT


#endif // CHUNKLISTMAP_HPP
