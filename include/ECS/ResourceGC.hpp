#ifndef RESOURCE_G_C_HPP
#define RESOURCE_G_C_HPP 1

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "cutil/HashHelper.hpp"
#include "defs.hpp"
#include <limits>

namespace ECS
{

class EntityComponentManager;

namespace internal {
    enum class GCFlag : uint8_t {
        NONE = 0x0,
        MARK = 0x1,
        ROOT = 0x2,
        LEAF = 0x4
    };

    inline constexpr GCFlag operator&(GCFlag x, GCFlag y) {
        return static_cast<GCFlag>(static_cast<uint8_t>(x) & static_cast<uint8_t>(y));
    }
    inline constexpr GCFlag operator|(GCFlag x, GCFlag y) {
        return static_cast<GCFlag>(static_cast<uint8_t>(x) | static_cast<uint8_t>(y));
    }
    inline constexpr GCFlag operator~(GCFlag x) {
        return static_cast<GCFlag>(~static_cast<uint8_t>(x));
    }
    inline GCFlag & operator&=(GCFlag & x, GCFlag y){ x = x & y;return x; }
    inline GCFlag & operator|=(GCFlag & x, GCFlag y) { x = x | y;return x; }

    static const uint32_t PRIMES_COUNT = 24;
    static const uint32_t primes[PRIMES_COUNT] = {
        0,       1,       5,       11,
        23,      53,      101,     197,
        389,     683,     1259,    2417,
        4733,    9371,    18617,   37097,
        74093,   148073,  296099,  592019,
        1100009, 2200013, 4400021, 8800019
    };
}

/// @brief stores set of values. garbage collects values with mark & sweep algorithm, searching inside ECMs.
class ResourceGC
{
public:
    using Type = intptr_t;
    static const uint32_t INVALID_INDEX = 0xFFFFFFFF;
    typedef void (dtor_fn)(Type);
protected:
    Type* values = nullptr;
    uint32_t* hashes = nullptr;
    dtor_fn** dtors = nullptr;
    internal::GCFlag* flags = nullptr;
    uint32_t _size = 0;
    uint32_t mitems = 0;
    uint32_t unoccupied = 0;
    Type minValue=std::numeric_limits<Type>::max();
    Type maxValue=std::numeric_limits<Type>::min();
    float loadFactor = 0.9;
    float sweepFactor = 0.5;

    // uint32_t hash(uint64_t ad) { return (uint32_t) ((13*ad) ^ (ad >> 15)); }

    uint32_t probe(uint32_t i, uint32_t h) const {
        int32_t v = (int32_t)(i - (h-1));
        if (v < 0)
            v = this->size() + v;
        return v;
    }
    uint32_t getIndex(Type value) const {
        uint32_t i, j, h;
        i = HashHelper::FNV1A32(value) % this->size(); j = 0;
        while (1) {
            h = this->hashes[i];
            if (h == 0 || j > this->probe(i, h))
                return INVALID_INDEX;
            if (this->values[i] == value)
                return i;
            i = (i+1) % this->size(); j++;
        }
        return INVALID_INDEX;
    }
    void addValue(dtor_fn* dtor, Type value, internal::GCFlag flag = internal::GCFlag::NONE)
    {
        uint32_t hash;

        dtor_fn* dtor_buffer;
        Type value_buffer;
        uint32_t hash_buffer;
        internal::GCFlag flag_buffer;

        uint32_t i, p, j, h;

        i = HashHelper::FNV1A32(value) % this->size(); j = 0;

        hash = i + 1;

        while (1) {
            h = this->hashes[i];
            if (h == 0) {
                this->values[i] = value;
                this->hashes[i] = hash;
                this->dtors[i] = dtor;
                this->flags[i] = flag;
                this->unoccupied--;
                return;
            }
            if (this->values[i] == value)
                return;
            p = this->probe(i, h);
            if (j >= p) {
                value_buffer = this->values[i];
                hash_buffer = this->hashes[i];
                dtor_buffer = this->dtors[i];
                flag_buffer = this->flags[i];

                this->values[i] = value;
                this->hashes[i] = hash;
                this->dtors[i] = dtor;
                this->flags[i] = flag;

                value=value_buffer;
                hash=hash_buffer;
                dtor=dtor_buffer;
                flag=flag_buffer;

                j = p;
            }
            i = (i+1) % this->size(); j++;
        }
    }
    void remIndex(uint32_t j) {
        uint32_t nj, nh;
        while (1) {
            nj = (j+1) % this->size();
            nh = this->hashes[nj];
            if (nh != 0 && this->probe(nj, nh) > 0) {
                this->values[j] = this->values[nj];
                this->hashes[j] = this->hashes[nj];
                this->dtors[j] = this->dtors[nj];
                this->flags[j] = this->flags[nj];
                j = nj;
            } else {
                break;
            }
        }
        this->hashes[j]=0;
        this->unoccupied++;
        return;
    }
    void remValue(Type value) {
        uint32_t i, j, h;
        if (this->occupiedNodes() == 0)
            return;
        i = HashHelper::FNV1A32(value) % this->size(); j = 0;
        while (1) {
            h = this->hashes[i];
            if (h == 0 || j > this->probe(i, h))
                return;
            if (this->values[i] == value) {
                remIndex(i);
                return;
            }
            i = (i+1) % this->size(); j++;
        }
    }

    uint32_t idealSize(uint32_t current_size) {
        uint32_t i, last;
        current_size = (uint32_t)((float)(current_size+1) / this->loadFactor);
        for (i = 0; i < internal::PRIMES_COUNT; i++)
            if (internal::primes[i] >= current_size)
                return internal::primes[i];
        last = internal::primes[internal::PRIMES_COUNT-1];
        for (i = 0;; i++)
            if (last * i >= current_size)
                return last * i;
        throw std::runtime_error("idealSize");
    }
    void appendFrom(const ResourceGC& src) {
        for (uint32_t offset = 0; offset < src.size(); ++offset)
            if (src.hashes[offset] != 0)
                addValue(src.dtors[offset],src.values[offset],src.flags[offset]);
    }
    void rehash(uint32_t size) {
        if (size < minimumSize())
            size = minimumSize();
        if (size == this->size())
            return;
    #ifdef DEBUG
        if(size < this->occupiedNodes())
            throw std::out_of_range("rehash(): size smaller than needed, causing infinite loop.");
    #endif
        ResourceGC temp(size);
        temp.appendFrom(*this);
        *this = std::move(temp);
    }
    void resizeMore() {
        const size_t new_size = this->idealSize(this->occupiedNodes());
        if(new_size > this->size())
            this->rehash(new_size);
    }
    void resizeLess() {
        const size_t new_size = this->idealSize(this->occupiedNodes());
        if(new_size < this->size())
            this->rehash(new_size);
    }
public:
    ResourceGC(uint32_t count = 0) {
        if (count < minimumSize())
            count = minimumSize();
        // isnt power of 2?
    #ifdef DEBUG
        if(count < 1)
            throw std::invalid_argument("ResourceGC(): array size must be atleast 1");
    #endif

        uint32_t offsets[5];
        offsets[0] = 0;
        offsets[1] = offsets[0] + alignTo64(sizeof(Type),count);
        offsets[2] = offsets[1] + alignTo64(sizeof(uint32_t),count);
        offsets[3] = offsets[2] + alignTo64(sizeof(dtor_fn*),count);
        offsets[4] = offsets[3] + alignTo64(sizeof(internal::GCFlag),count);

        uint8_t * const ptr = allocator<uint8_t>().allocate(offsets[4]);

        // must be: ptr + 0
        values = (Type*)    (ptr+offsets[0]);
        hashes = (uint32_t*)(ptr+offsets[1]);
        dtors  = (dtor_fn**)(ptr+offsets[2]);
        flags  = (internal::GCFlag*)  (ptr+offsets[3]);
        _size = unoccupied = count;

        memset(this->hashes,0,alignTo64(sizeof(uint32_t),count));
        memset(flags,0,sizeof(internal::GCFlag)*count);
    }
    ResourceGC(const ResourceGC&) = delete;
    ResourceGC(ResourceGC&& v) {
        *this = std::move(v);
    }
    ResourceGC& operator=(const ResourceGC&) = delete;
    ResourceGC& operator=(ResourceGC&& v) {
        if(this != &v){
            if(this->values)
                allocator<uint8_t>().deallocate((uint8_t*)(this->values));
            this->values = v.values;
            this->hashes = v.hashes;
            this->dtors = v.dtors;
            this->flags = v.flags;
            this->unoccupied = v.unoccupied;
            this->_size = v._size;
            memset(&v,0,sizeof(v));
        }
        return *this;
    }
    ~ResourceGC(){
        for(uint32_t index=0;index<_size;++index)
            if(hashes[index] != 0)
                this->dtors[index](values[index]);
        allocator<uint8_t>().deallocate(values);
    }

    inline uint32_t size() const { return this->_size; }
    inline uint32_t unoccupiedNodes() const { return unoccupied; }
    inline uint32_t occupiedNodes() const { return size() - unoccupiedNodes();}
    inline bool isEmpty() const { return occupiedNodes() == 0; }
    static inline uint32_t minimumSize() { return 1; }
    inline bool contains(Type value) const { return getIndex(value) != INVALID_INDEX; }
    bool recommendGC() const { return this->occupiedNodes() > this->mitems; }


protected:
    void markValue(const Type value) {
        if (value < this->minValue || this->maxValue < value)
            return;
        uint32_t i, j, h;
        i = HashHelper::FNV1A32(value) % this->size(); j = 0;
        while (1) {
            h = this->hashes[i];
            if (h == 0 || j > this->probe(i, h))
                return;
            if (value == this->values[i]) {
                if ((this->flags[i] & internal::GCFlag::MARK) != internal::GCFlag::NONE)
                    return;
                this->flags[i] |= internal::GCFlag::MARK;
            }
            i = (i+1) % this->size(); j++;
        }
    }
public:
    void searchMemory(const uint8_t *region,const uint32_t regionSizeInByte) {
        for (uint32_t k = 0; k < regionSizeInByte/sizeof(Type); k++)
            markValue(((const Type*)region)[k]);
    }
    void sweep() {
        uint32_t i;
        if (this->isEmpty()) { return; }
        for (i = 0; i < this->size(); i++) {
            if (this->hashes[i] == 0) { continue; }
            if ((this->flags[i] & internal::GCFlag::MARK) != internal::GCFlag::NONE) { continue; }
            if ((this->flags[i] & internal::GCFlag::ROOT) != internal::GCFlag::NONE) { continue; }
            this->dtors[i](this->values[i]);
        }
        for (i = 0; i < this->size();) {
            if (this->hashes[i] == 0) { i++;continue; }
            if ((this->flags[i] & internal::GCFlag::MARK) != internal::GCFlag::NONE) { i++;continue; }
            if ((this->flags[i] & internal::GCFlag::ROOT) != internal::GCFlag::NONE) { i++;continue; }
            remIndex(i);
        }
        for (i = 0; i < this->size();i++)
            if ((this->flags[i] & internal::GCFlag::MARK) != internal::GCFlag::NONE)
                this->flags[i] &= ~internal::GCFlag::MARK;
        this->resizeLess();
        this->mitems = this->occupiedNodes() + (size_t)(this->occupiedNodes() * this->sweepFactor) + 1;
    }
    void remove(Type value) {
        this->remValue(value);
        this->resizeLess();
        this->mitems = this->occupiedNodes() + this->occupiedNodes() / 2 + 1;
    }
    void add(dtor_fn* dtor, Type value, internal::GCFlag flag = internal::GCFlag::NONE){
        this->maxValue = std::max(this->maxValue,value);
        this->minValue = std::min(this->minValue,value);
        this->resizeMore();
        this->addValue(dtor, value, flag);
    }
};

}

#endif
