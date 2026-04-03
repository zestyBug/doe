#if !defined(RESOURCEMAP_HPP)
#define RESOURCEMAP_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    /// @brief A simple integer to integer proxy
    class ResourceMap final
    {
    protected:
        static constexpr uint32_t _AValidHashCode = 0x00000001;
        /// @brief When current hash value is invalid but you should not stop searching.
        static constexpr uint32_t _SkipCode = 0xFFFFFFFF;

        align_ptr<uint32_t[]> keys{};
        align_ptr<void*   []> values{};
        uint32_t _capacity = 0;
        uint32_t emptyNodes=0;
        uint32_t skipNodes=0;

        void resize(uint32_t size){
            if (size < minimumSize())
                size = minimumSize();
            if (size == this->capacity())
                return;
            ResourceMap temp;
            temp.init(size);
            temp.appendFrom(*this);
            *this = std::move(temp);
        }

    public:

        ResourceMap() = default;
        ResourceMap(const ResourceMap&) = delete;
        ResourceMap(ResourceMap&& v)
            :keys{std::move(v.keys)},values{std::move(v.values)}
        {
            this->_capacity = v._capacity;
            this->emptyNodes = v.emptyNodes;
            this->skipNodes = v.skipNodes;
            v._capacity=0;
            v.emptyNodes=0;
            v.skipNodes=0;
        }
        ResourceMap& operator=(const ResourceMap&) = delete;
        ResourceMap& operator=(ResourceMap&& v){
            if(this != &v){
                this->keys     = std::move(v.keys);
                this->values     = std::move(v.values);
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
            return 64 / sizeof(void*);
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
        void init(uint32_t count = 0){
            if (count < minimumSize())
                count = minimumSize();

            // is power of 2?
            if(0 != (count & (count - 1)))
                throw std::invalid_argument("Init(): count must be power of 2");

            keys = make_align<uint32_t[]>(count);
            memset(keys.get(),0,count * sizeof(uint32_t));
            values.reset((void**)allocator<>().allocate(sizeof(void*)*count));
            _capacity = count;
            emptyNodes = count;
            skipNodes = 0;
        }
        void appendFrom(ResourceMap& src){
            for (uint32_t offset = 0; offset < src.capacity(); ++offset)
            {
                uint32_t key = src.keys[offset];
                if (key != 0 && key != _SkipCode)
                    set(key, src.values[offset]);
            }
        }
        // the whole popuse is to free space when object is unused but still in memory
        void reset(){
            keys.reset();
            values.reset();
            emptyNodes=0;
            skipNodes=0;
        }
        void set(uint32_t key, void* value) {
            if(key == 0 || key == _SkipCode)
                throw std::runtime_error("set(): invalid key");
            uint32_t offset = key & hashMask();
            uint32_t attempts = 0;
            while (true)
            {
                const uint32_t ikey = keys[offset];
                if (ikey == 0)
                {
                    keys[offset] = key;
                    values[offset] = value;
                    --emptyNodes;
                    possiblyGrow();
                    return;
                }
                if (ikey == _SkipCode)
                {
                    keys[offset] = key;
                    values[offset] = value;
                    --skipNodes;
                    possiblyGrow();
                    return;
                }
                if(ikey == key) {
                    values[offset] = value;
                    return;
                }
                offset = (offset + 1) & hashMask();
                ++attempts;
                if(attempts >= capacity())
                    // we should nor reach here, a possiblyGrow() call must prevent it
                    throw std::runtime_error("add(): something went wrong");
            }
        }
        void remove(uint32_t key)
        {
            int32_t offset = indexOf(key);
            if(offset < 0)
                throw std::runtime_error("remove(): invalid key");
            else if((uint32_t)offset >= this->capacity())
                throw std::runtime_error("remove(): internall error");
            keys[offset] = _SkipCode;
            ++skipNodes;
            possiblyShrink();
        }
        int indexOf(uint32_t key) const {
            if(key == 0 || key == _SkipCode)
                throw std::runtime_error("set(): invalid key");
            uint32_t offset = (key & hashMask());
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t ikey = keys[offset];
                if (ikey == 0)
                    return -1;
                if (ikey == key)
                    return offset;
                offset = (offset + 1) & hashMask();
                ++attempts;
                if (attempts == capacity())
                    return -1;
            }
        }
        /// @brief find a pointer with a key using hash list
        /// @return value or nullptr
        void* tryGet(uint32_t key) const {
            int32_t offset = indexOf(key);
            if(offset < 0)
                return nullptr;
            else if((uint32_t)offset >= this->capacity())
                throw std::runtime_error("remove(): internall error");
            return values[offset];
        }
    };
}
#endif