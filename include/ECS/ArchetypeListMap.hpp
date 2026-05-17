#if !defined(ARCHETYPELISTMAP_HPP)
#define ARCHETYPELISTMAP_HPP

#include "cutil/basics.hpp"
#include "cutil/HashHelper.hpp"

namespace ECS
{
    struct Archetype;
    struct TypeID;
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
        static uint32_t getHashCode(const_span<TypeID> type);

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

            const uint32_t size1 = alignPointerSize(sizeof(Archetype*)*count);
            const uint32_t size2 = sizeof(uint32_t)*count;
            uint8_t* ptr = allocator().allocate(size1+size2);
            pointers.reset((Archetype**)ptr);
            hashes = (uint32_t *)(ptr+size1);
            memset(hashes,0,size2);
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
        void add(Archetype* ptr);
        void remove(Archetype* ptr);
        /// @brief find a pointer with a key using hash list
        /// @return value or nullptr
        Archetype* tryGet(const_span<ECS::TypeID> key) const;
        int32_t indexOf(Archetype* ptr) const;
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