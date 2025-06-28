#if !defined(MAP_HPP)
#define MAP_HPP

#include "basics.hpp"
#include "HashHelper.hpp"
#include <vector>

// TODO: add custome allocator like:  std::pair<std::allocator<uint32_t>,std::allocator<_Tp*>>

template <typename _Key, typename _Tp>
class map
{
protected:
    static const uint32_t _AValidHashCode = 0x00000001;
    static const uint32_t _SkipCode = 0xFFFFFFFF;
    /// @brief generates hash code for a pointer
    /// @param types note that type flags are also included, like disable-ness, prefab-being
    /// @return (may-modified) hash value

    std::vector<uint32_t,allocator<uint32_t>> hashes{};
    std::vector<_Tp*,allocator<_Tp*>> pointers{};
    uint32_t emptyNodes=0;
    uint32_t skipNodes=0;

    static uint32_t getHashCode(_Key key)
    {
        uint32_t result = HashHelper::FNV1A32(key);
        if (result == 0 || result == _SkipCode)
            result = _AValidHashCode;
        return result;
    }
public:

        map() = default;
        map(const map&) = delete;
        map(map&& v)
            :hashes{std::move(v.hashes)},pointers{std::move(v.pointers)}
        {
            this->emptyNodes = v.emptyNodes;
            this->skipNodes = v.skipNodes;
            v.emptyNodes=0;
            v.skipNodes=0;
        }
        map& operator=(const map&) = delete;
        map& operator=(map&& v){
            if(this != &v){
                this->hashes     = std::move(v.hashes);
                this->pointers = std::move(v.pointers);
                this->emptyNodes = v.emptyNodes;
                this->skipNodes = v.skipNodes;
                v.emptyNodes=0;
                v.skipNodes=0;
            }
            return *this;
        }
        inline uint32_t size() const {
            return (uint32_t) hashes.size();
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


        void init(uint32_t count){
            if (count < minimumSize())
                count = minimumSize();

            // is power of 2?
            if(0 != (count & (count - 1)))
                throw std::invalid_argument("Init(): count must be power of 2");

            hashes.resize(count);
            pointers.resize(count);

            emptyNodes = count;
            skipNodes = 0;
        }
        void appendFrom(map& src){
            for (uint32_t offset = 0; offset < src.size(); ++offset)
            {
                uint32_t hash = src.hashes[offset];
                if (hash != 0 && hash != _SkipCode)
                    add(src.pointers[offset]);
            }
            src.reset();
        }
        void resize(uint32_t size){
            if (size < minimumSize())
                size = minimumSize();
            if (size == this->size())
                return;
            map temp;
            temp.init(size);
            temp.appendFrom(*this);
            *this = std::move(temp);
        }
        
        void setCapacity(uint32_t capacity){
            if (capacity < minimumSize())
                capacity = minimumSize();
            hashes.resize(capacity);
            pointers.resize(capacity);
        }
        void add(_Tp* ptr) {
            if(ptr == nullptr)
                throw std::invalid_argument("add(): null pointer");
            uint32_t desiredHash = ptr->getHash();
            uint32_t offset = (int)(desiredHash & hashMask());
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes.at(offset);
                if (hash == 0)
                {
                    hashes[offset] = desiredHash;
                    pointers[offset] = ptr;
                    --emptyNodes;
                    possiblyGrow();
                    return;
                }

                if (hash == _SkipCode)
                {
                    hashes[offset] = desiredHash;
                    pointers[offset] = ptr;
                    --skipNodes;
                    possiblyGrow();
                    return;
                }

                if(unlikely(hash == desiredHash)) {
                    if(pointers[offset] == ptr)
                        throw std::invalid_argument("add(): adding duplicated item");
                }

                offset = (offset + 1) & hashMask();
                ++attempts;
                if(attempts >= size())
                    // we should nor reach here, a possiblyGrow() call must prevent it
                    throw std::runtime_error("add(): something wet wrong");
            }
        }
        void remove(_Tp* ptr){
            int32_t offset = indexOf(ptr);
            if(offset < 0)
                throw std::runtime_error("remove(): pointer not found");
            hashes[offset] = _SkipCode;
            ++skipNodes;
            possiblyShrink();
        }
        /// @brief find a pointer with a key using hash list
        /// @return value or nullptr
        _Tp* tryGet(_Key key) const {
            uint32_t desiredHash = getHashCode(key);
            uint32_t offset = desiredHash & hashMask();
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes.at(offset);
                if (hash == 0)
                    return nullptr;
                if (hash == desiredHash)
                {
                    _Tp *ptr = pointers[offset];
                    if (*ptr == key)
                        return ptr;
                }
                offset = (offset + 1) & hashMask();
                ++attempts;
                if (attempts == size())
                    return nullptr;
            }
        }
        int indexOf(_Tp* ptr) const {
            if(ptr == nullptr)
                return -1;
            uint32_t desiredHash = ptr->getHash();
            uint32_t offset = (desiredHash & hashMask());
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes[offset];
                if (hash == 0)
                    return -1;
                if (hash == desiredHash)
                {
                    if (pointers[offset] == ptr)
                        return offset;
                }
                offset = (offset + 1) & hashMask();
                ++attempts;
                if (attempts == size())
                    return -1;
            }
        }
        bool contains(_Tp* ptr) const {
            return indexOf(ptr) != -1;
        }
        // the whole popuse is to free space when object is unused but still in memory
        void reset(){
            hashes = std::vector<uint32_t,allocator<uint32_t>>();
            pointers = std::vector<_Tp*,allocator<_Tp*>>();
            emptyNodes=0;
            skipNodes=0;
        }
};

#endif // MAP_HPP
