#if !defined(SET_HPP)
#define SET_HPP

#include "basics.hpp"
#include "span.hpp"
#include "cutil/HashHelper.hpp"

/**
 * @brief a hash set on vector, containing unique values
 * suitable for small structures (uses lots of copy operations)
 * @warning without calling destruction
 */
template <typename Type>
class set
{
protected:
    static const Hash32 _AValidHashCode = 2;
    static const Hash32 _SkipCode = 1;
    static const Hash32 _InvalidHashCode = 0;

    std::unique_ptr<Hash32[]> hashes;
    Type    *values = nullptr;
    uint32_t capacity = 0;
    uint32_t unoccupied = 0;

public:

    set(uint32_t count = 0) {
        if (count < minimumSize())
            count = minimumSize();

        // is power of 2?
        if(0 != (count & (count - 1)))
            throw std::invalid_argument("Init(): count must be power of 2");

        const uint32_t size1 = alignPointerSize(sizeof(uint32_t)*count);
        const uint32_t size2 = sizeof(Type)*count;
        uint8_t* ptr = std::allocator<uint8_t>().allocate(size1+size2);
        hashes.reset((uint32_t*)ptr);
        values = (Type*)(ptr + size1);
        memset(ptr,0,size1 + size2);
        capacity = count;
        unoccupied = count;
    }
    set(const set&) = delete;
    set(set&& v): hashes{std::move(v.hashes)},values{std::move(v.values)} {
        this->capacity = v.capacity;
        this->unoccupied = v.unoccupied;
        v.capacity=0;
        v.unoccupied=0;
    }
    ~set(){
        hashes.reset();
    }
    set& operator=(const set&) = delete;
    set& operator=(set&& v){
        if(this != &v){
            this->hashes = std::move(v.hashes);
            this->values = std::move(v.values);
            this->capacity = v.capacity;
            this->unoccupied = v.unoccupied;
            v.capacity=0;
            v.unoccupied=0;
        }
        return *this;
    }
    inline uint32_t unoccupiedNodes() const { return unoccupied; }
    inline uint32_t occupiedNodes() const { return capacity - unoccupiedNodes(); }
    inline bool isEmpty() const { return occupiedNodes() == 0; }
    /// @brief ! suppose size is power of 2
    inline uint32_t hashMask() const { return capacity - 1; }
    inline uint32_t minimumSize() const { return 64 / sizeof(uint32_t); }
    inline void possiblyGrow() { if (unoccupiedNodes() < capacity / 3) resize(capacity * 2); }
    inline void possiblyShrink() { if (occupiedNodes() < capacity / 3) resize(capacity / 2); }
    void appendFrom(const set& src){
        for (uint32_t offset = 0; offset < capacity; ++offset)
        {
            Hash32 hash = src.hashes[offset];
            if (hash != _InvalidHashCode && hash != _SkipCode)
                insert(hash, src.values[offset]);
        }
    }
    void resize(uint32_t size){
        if (size < minimumSize())
            size = minimumSize();
        if (size == capacity)
            return;
        set temp{size};
        temp.appendFrom(*this);
        *this = std::move(temp);
    }
    void insert(Hash32 hashValue, Type value) {
        if(hashValue == _InvalidHashCode || hashValue == _SkipCode)
            hashValue = _AValidHashCode;
        uint32_t offset = (int)(hashValue & hashMask());
        uint32_t attempts = 0;
        while (true)
        {
            Hash32 hash = hashes[offset];
            if(hash == hashValue)
                throw std::invalid_argument("insert(): hash already exists");

            if (hash == _InvalidHashCode || hash == _SkipCode)
            {
                hashes[offset] = hashValue;
                values[offset] = value;
                unoccupied--;
                possiblyGrow();
                return;
            }

            offset = (offset + 1) & hashMask();
            ++attempts;
            if(attempts >= capacity)
                // we should nor reach here, a possiblyGrow() call must prevent it
                throw std::runtime_error("add(): something went wrong");
        }
    }
    void remove(Hash32 hashValue){
        if(hashValue == _InvalidHashCode || hashValue == _SkipCode)
            hashValue = _AValidHashCode;
        int32_t offset = indexOf(hashValue);
        if(offset < 0)
            throw std::runtime_error("remove(): value not found");
        hashes[offset] = _SkipCode;
        unoccupied++;
        possiblyShrink();
    }
    Type get(Hash32 hashValue) const {
        if(hashValue == _InvalidHashCode || hashValue == _SkipCode)
            hashValue = _AValidHashCode;
        uint32_t offset = (hashValue & hashMask());
        uint32_t attempts = 0;
        while (true)
        {
            Hash32 hash = hashes[offset];
            if (hash == _InvalidHashCode)
                throw std::runtime_error("get(): not found");
            if (hash == hashValue)
                return values[offset];
            offset = (offset + 1) & hashMask();
            ++attempts;
            if (attempts == capacity)
                throw std::runtime_error("get(): not found");
        }
    }
    Type getValue(uint32_t index) const {
        if (index >= capacity)
            throw std::out_of_range("getValue(): not found");
        if (hashes[index] == _InvalidHashCode || hashes[index] == _SkipCode)
            throw std::invalid_argument("getValue(): not found");
        return values[index];
    }
    /// @brief index of an hash value or -1 if not found 
    int indexOf(Hash32 hashValue) const {
        if(hashValue == _InvalidHashCode || hashValue == _SkipCode)
            hashValue = _AValidHashCode;
        uint32_t offset = (hashValue & hashMask());
        uint32_t attempts = 0;
        while (true)
        {
            Hash32 hash = hashes[offset];
            if (hash == _InvalidHashCode)
                return -1;
            if (hash == hashValue)
            {
                return offset;
            }
            offset = (offset + 1) & hashMask();
            ++attempts;
            if (attempts == capacity)
                return -1;
        }
    }
    void reset(){
        hashes.reset();
        values = nullptr;
        capacity = 0;
        unoccupied=0;
    }
    struct Iterator {
        Hash32 *hash;
        Type *value;
        Type operator*() const {
            return *value;
        }
        operator bool() const {
            return *hash != _InvalidHashCode && *hash != _SkipCode;
        }
        void operator++(){
            hash++;
            value++;
        }
        bool operator != (const Iterator& v) const {
            return this->hash != v.hash;
        }
    };
    Iterator begin(){
        return Iterator{hashes.get(),values};
    }
    const Iterator end() {
        return Iterator{hashes.get()+capacity,values+capacity};
    }
};

#endif // SET_HPP
