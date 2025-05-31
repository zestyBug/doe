#if !defined(SET_HPP)
#define SET_HPP

#include "basics.hpp"
#include "span.hpp"
#include "cutil/HashHelper.hpp"

/**
 * @brief a hash set on vector, containing unique values
 * suitable for small structures (uses lots of copy constructor)
 */
template <typename Type>
class set
{
protected:
    static const uint32_t _AValidHashCode = 2;
    // must be 1 otherwise it will fail
    static const uint32_t _SkipCode = 1;

    std::vector<uint32_t,allocator<uint32_t>> hashes{};
    std::vector<Type,allocator<Type>> values{};
    // [emptyNodes,skipNodes]
    uint32_t unoccupied[2] = {0,0};

    static uint32_t getHashCode(Type key)
    {
        uint32_t result;
        if(sizeof(Type) <= sizeof(uint32_t))   
            result = static_cast<uint32_t>(key);
        else
            result = HashHelper::FNV1A32(key);

        if (result == 0 || result == _SkipCode)
            result = _AValidHashCode;
        return result;
    }
public:

        set(uint32_t count = 0) {
            if (count < minimumSize())
                count = minimumSize();

            // is power of 2?
            if(0 != (count & (count - 1)))
                throw std::invalid_argument("Init(): count must be power of 2");

            hashes.resize(count);
            values.resize(count);

            unoccupied[0] = count;
            unoccupied[1] = 0;
        }
        set(const set&) = delete;
        set(set&& v): hashes{std::move(v.hashes)},values{std::move(v.values)} {
            this->unoccupied[0] = v.unoccupied[0];
            this->unoccupied[1] = v.unoccupied[1];
            v.unoccupied[0]=0;
            v.unoccupied[1]=0;
        }
        set& operator=(const set&) = delete;
        set& operator=(set&& v){
            if(this != &v){
                this->hashes = std::move(v.hashes);
                this->values = std::move(v.values);
                this->unoccupied[0] = v.unoccupied[0];
                this->unoccupied[1] = v.unoccupied[1];
                v.unoccupied[0]=0;
                v.unoccupied[1]=0;
            }
            return *this;
        }
        inline uint32_t size() const { return (uint32_t) hashes.size(); }
        inline uint32_t unoccupiedNodes() const { return unoccupied[0] + unoccupied[1]; }
        inline uint32_t occupiedNodes() const { return size() - unoccupiedNodes(); }
        inline bool isEmpty() const { return occupiedNodes() == 0; }
        /// @brief ! suppose size is power of 2
        inline uint32_t hashMask() const { return size() - 1; }
        inline uint32_t minimumSize() const { return 64 / sizeof(uint32_t); }
        inline void possiblyGrow() { if (unoccupiedNodes() < size() / 3) resize(size() * 2); }
        inline void possiblyShrink() { if (occupiedNodes() < size() / 3) resize(size() / 2); }
        void appendFrom(const set& src){
            for (uint32_t offset = 0; offset < src.size(); ++offset)
            {
                uint32_t hash = src.hashes[offset];
                if (hash != 0 && hash != _SkipCode)
                    insert(src.values[offset]);
            }
        }
        void resize(uint32_t size){
            if (size < minimumSize())
                size = minimumSize();
            if (size == this->size())
                return;
            set temp;
            temp.init(size);
            temp.appendFrom(*this);
            *this = std::move(temp);
        }
        void setCapacity(uint32_t capacity){
            if (capacity < minimumSize())
                capacity = minimumSize();
            hashes.resize(capacity);
            values.resize(capacity);
        }
        void insert(Type value) {
            uint32_t desiredHash = getHashCode(value);
            uint32_t offset = (int)(desiredHash & hashMask());
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes.at(offset);
                if(hash == desiredHash) {
                    if(value[offset] == value)
                        return;
                }
                
                if (hash == 0 || hash == _SkipCode)
                {
                    hashes[offset] = desiredHash;
                    values[offset] = value;
                    // micro optimized!
                    unoccupied[hash]--;
                    possiblyGrow();
                    return;
                }

                offset = (offset + 1) & hashMask();
                ++attempts;
                if(attempts >= size())
                    // we should nor reach here, a possiblyGrow() call must prevent it
                    throw std::runtime_error("add(): something wet wrong");
            }
        }
        void remove(Type value){
            int32_t offset = indexOf(value);
            if(offset != -1)
                throw std::runtime_error("remove(): archtype not found");
            hashes[offset] = _SkipCode;
            unoccupied[1]--;
            possiblyShrink();
        }
        int indexOf(Type value) const {
            uint32_t desiredHash = getHashCode(value);
            uint32_t offset = (desiredHash & hashMask());
            uint32_t attempts = 0;
            while (true)
            {
                uint32_t hash = hashes[offset];
                if (hash == 0)
                    return -1;
                if (hash == desiredHash)
                {
                    if (values[offset] == value)
                        return offset;
                }
                offset = (offset + 1) & hashMask();
                ++attempts;
                if (attempts == size())
                    return -1;
            }
        }
        bool contains(Type value) const {
            return indexOf(value) != -1;
        }
        // the whole popuse is to free space when object is unused but still in memory
        void reset(){
            hashes = std::vector<uint32_t,allocator<uint32_t>>();
            values = std::vector<Type,allocator<Type>>();
            memset(unoccupied,0,sizeof(unoccupied));
        }
};

/**
 * @brief a integer set on vector, containing unique values except default value as reserved!
 * suitable for simple data types like int (uses lots of copy constructor)
 */
template <typename Type>
class vector_set
{
protected:
    // std::vector allocates array with default value,
    // so we consider default value as unallocated space
    const Type invalidValue = Type();
    std::vector<Type,allocator<Type>> values{};
    uint32_t unoccupied = 0;
public:
    vector_set(uint32_t count = 0){
        if (count < minimumSize())
            count = minimumSize();

        // is power of 2?
        if(0 != (count & (count - 1)))
            throw std::invalid_argument("Init(): count must be power of 2");

        values.resize(count);

        unoccupied = count;
    }
    vector_set(const vector_set&) = delete;
    vector_set(vector_set&& v){
        new (this) vector_set();
        *this = std::move(v);
    }
    vector_set& operator=(const vector_set&) = delete;
    vector_set& operator=(vector_set&& v){
        if(this != &v){
            this->values = std::move(v.values);
            unoccupied = v.unoccupied;
            v.unoccupied = 0;
        }
        return *this;
    }
    inline uint32_t size() const {
        return (uint32_t) values.size();
    }
    inline uint32_t unoccupiedNodes() const {
        return unoccupied;
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
        return std::max<uint32_t>(64 / sizeof(Type),4);
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

    void appendFrom(const vector_set& src){
        for (uint32_t offset = 0; offset < src.size(); ++offset)
        {
            uint32_t value = src.values[offset];
            if (value != 0)
                insert(value);
        }
    }
    void resize(uint32_t size) {
        if (size < minimumSize())
            size = minimumSize();
        if (size == this->size())
            return;
        vector_set temp(size);
        temp.appendFrom(*this);
        *this = std::move(temp);
    }
    
    void setCapacity(uint32_t capacity){
        if (capacity < minimumSize())
            capacity = minimumSize();
        values.resize(capacity);
    }
    void insert(Type value) {
        if(value == invalidValue)
            throw std::invalid_argument("insert(): invalide argument");
        uint32_t hmask = hashMask();
        uint32_t offset = (int)(value & hmask);
        uint32_t attempts = 0;
        while (true)
        {
            Type v = values[offset];
            if(v == value)
                return;
            if (v == invalidValue)
            {
                values[offset] = value;
                unoccupied--;
                possiblyGrow();
                return;
            }

            offset = (offset + 1) & hashMask();
            ++attempts;
            if(attempts >= size())
                // we should nor reach here, a possiblyGrow() call must prevent it
                throw std::runtime_error("add(): something wet wrong");
        }
    }
    int indexOf(Type value) const {
        if(value == invalidValue)
            throw std::invalid_argument("insert(): invalide argument");
        uint32_t offset = (value & hashMask());
        uint32_t attempts = 0;
        while (true)
        {
            uint32_t v = values[offset];
            if (v == value)
                return offset;
            offset = (offset + 1) & hashMask();
            ++attempts;
            if (attempts == size())
                return -1;
        }
    }
    void remove(Type value){
        int32_t offset = indexOf(value);
        if(offset != -1)
            throw std::runtime_error("remove(): archtype not found");
        values[offset] = invalidValue;
        unoccupied--;
        possiblyShrink();
    }
    bool contains(Type value) const {
        return indexOf(value) != -1;
    }
    // the whole popuse is to free space when object is unused but still in memory
    void reset(){
        values = std::vector<Type,allocator<Type>>();
        unoccupied=0;
    }
    struct iterator{
        const Type invalidValue = Type();
        span<Type> ptr;
        iterator(span<Type> addr):ptr{addr}{
            this->findNextValue();
        }
        inline void findNextValue(){
            uint32_t i = 0;
            for (; i < ptr.size(); i++)
                if(ptr[i] != invalidValue)
                    break;
            ptr += i;
        }
        iterator& operator ++(){
            ++ptr;
            findNextValue();
            return *this;
        }
        Type operator*(){
            return ptr.at(0);
        }
        bool operator != (const iterator&){
            if(ptr.empty())
                return false;
            return true;
        }
        bool operator == (const iterator&){
            if(ptr.empty())
                return true;
            return false;
        }
    };
    iterator begin(){
        return iterator(this->values);
    }
    const iterator end(){
        return iterator(this->values);
    }
};

#endif // SET_HPP
