#if !defined(ARCHETYPELISTMAP_HPP)
#define ARCHETYPELISTMAP_HPP

#include "cutil/basics.hpp"
#include "cutil/HashHelper.hpp"
#include "Archetype.hpp"

namespace ECS
{
    class ArchetypeListMap
    {
    protected:
        static constexpr uint32_t _AValidHashCode = 0x00000001;
        static constexpr uint32_t _SkipCode = 0xFFFFFFFF;
        /// @brief generates hash code for a pointer
        /// @param types note that type flags are also included, like disable-ness, prefab-being
        /// @return (may-modified) hash value

        align_ptr<Archetype*[]> pointers{};
        uint32_t* hashes = nullptr;
        uint32_t _capacity = 0;
        uint32_t emptyNodes=0;

    public:
        static uint32_t getHashCode(const_span<ECS::TypeID> type)
        {
            uint32_t result = HashHelper::FNV1A32(type);
            if (result == 0 || result == _SkipCode)
                result = _AValidHashCode;
            return result;
        }

        ArchetypeListMap() = default;
        ArchetypeListMap(const ArchetypeListMap&) = delete;
        ArchetypeListMap(ArchetypeListMap&& v)
            :pointers{std::move(v.pointers)}
        {
            this->hashes = v.hashes;
            this->_capacity = v._capacity;
            this->emptyNodes = v.emptyNodes;
            v.hashes=nullptr;
            v._capacity=0;
            v.emptyNodes=0;
        }
        ArchetypeListMap& operator=(const ArchetypeListMap&) = delete;
        ArchetypeListMap& operator=(ArchetypeListMap&& v){
            if(this != &v){
                this->pointers = std::move(v.pointers);
                this->hashes = v.hashes;
                this->_capacity = v._capacity;
                this->emptyNodes = v.emptyNodes;
                v.hashes=nullptr;
                v._capacity=0;
                v.emptyNodes=0;
            }
            return *this;
        }
        inline uint32_t size() const {
            return _capacity;
        }
        inline uint32_t unoccupiedNodes() const {
            return emptyNodes;
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

            const uint32_t size1 = alignTo64(sizeof(uint32_t),count);
            const uint32_t size2 = alignTo64(sizeof(Archetype*),count);
            uint8_t* ptr = allocator<>().allocate(size1+size2);
            pointers.reset((Archetype**)ptr);
            hashes = (uint32_t *)(ptr+size2);
            memset(hashes,0,size1);
            _capacity = count;
            emptyNodes = count;
        }
        void appendFrom(ArchetypeListMap& src){
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
            ArchetypeListMap temp;
            temp.init(size);
            temp.appendFrom(*this);
            *this = std::move(temp);
        }
        void add(Archetype* ptr) {
            if(ptr == nullptr)
                throw std::invalid_argument("add(): null pointer");
            uint32_t desiredHash = getHashCode(ptr->getType());
            uint32_t offset = desiredHash & hashMask();
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes[offset];
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
                    --emptyNodes;
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
                    throw std::runtime_error("add(): something went wrong");
            }
        }
        void remove(Archetype* ptr){
            int32_t offset = indexOf(ptr);
            if(offset < 0)
                throw std::runtime_error("remove(): pointer not found");
            hashes[offset] = _SkipCode;
            ++emptyNodes;
            possiblyShrink();
        }
        /// @brief find a pointer with a key using hash list
        /// @return value or nullptr
        Archetype* tryGet(const_span<ECS::TypeID> key) const {
            uint32_t desiredHash = getHashCode(key);
            uint32_t offset = desiredHash & hashMask();
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes[offset];
                if (hash == 0)
                    return nullptr;
                if (hash == desiredHash)
                {
                    Archetype *ptr = pointers[offset];
                    if (ptr->getType() == key)
                        return ptr;
                }
                offset = (offset + 1) & hashMask();
                ++attempts;
                if (attempts == size())
                    return nullptr;
            }
        }
        int indexOf(Archetype* ptr) const {
            if(ptr == nullptr)
                return -1;
            uint32_t desiredHash = getHashCode(ptr->getType());
            uint32_t offset = desiredHash & hashMask();
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
        bool contains(Archetype* ptr) const {
            return indexOf(ptr) != -1;
        }
        // the whole popuse is to free space when object is unused but still in memory
        void reset(){
            pointers.reset();
            hashes = nullptr;
            emptyNodes=0;
        }
    };
}
#endif