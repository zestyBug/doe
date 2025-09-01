#include "ECS/ResourceGC.hpp"
using namespace ECS;

static const uint32_t PRIMES_COUNT = 24;
/// @brief list of largest prime number smaller that fibonacci sequence(i) * 8
/// @note fibonacci sequence is better than doubling space in theory
static const uint32_t primes[PRIMES_COUNT] = {
    0,    7,     13,    24,
    37,   61,    103,   167,
    271,  439,   709,   1151,
    1861, 3011,  4877,  7883,
    12763,20663, 33427, 54101,
    87559,141679,229253,370919
};
uint32_t ResourceGC::probe(uint32_t i, uint32_t h) const {
    int32_t v = (int32_t)(i - (h-1));
    if (v < 0)
        v = this->size() + v;
    return v;
}
uint32_t ResourceGC::getIndex(Type value) const {
    uint32_t i, j, h;
    i = HashHelper::tgc_hash(value) % this->size(); j = 0;
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
void ResourceGC::addValue(dtor_fn* dtor, Type value, internal::GCFlag flag) {
    uint32_t hash;

    dtor_fn* dtor_buffer;
    Type value_buffer;
    uint32_t hash_buffer;
    internal::GCFlag flag_buffer;

    uint32_t i, p, j, h;

    i = HashHelper::tgc_hash(value) % this->size(); j = 0;
    // NOTE: this->size() must always be smaller than UINT32_MAX in order to hash works currectly
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
        // NOTE: changed from ">=" to ">", must be investigated more
        if (j > p) {
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
void ResourceGC::remIndex(uint32_t j) {
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
void ResourceGC::remValue(Type value) {
    uint32_t i, j, h;
    if (this->occupiedNodes() == 0)
        return;
    i = HashHelper::tgc_hash(value) % this->size(); j = 0;
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
uint32_t ResourceGC::idealSize(uint32_t current_size) {
    uint32_t i, last;
    current_size = (uint32_t)((float)(current_size+1) / this->loadFactor);
    for (i = 0; i < PRIMES_COUNT; i++)
        if (primes[i] >= current_size)
            return primes[i];
    last = primes[PRIMES_COUNT-1];
    for (i = 0;; i++)
        if (last * i >= current_size)
            return last * i;
    throw std::runtime_error("idealSize");
}
void ResourceGC::rehash(uint32_t size) {
    if (size < minimumSize())
        size = minimumSize();
    if (size == this->size())
        return;
#ifdef DEBUG
    if(size < this->occupiedNodes())
        throw std::out_of_range("rehash(): size smaller than needed, causing infinite loop.");
#endif
    ResourceGC GC{size};
    for (uint32_t offset = 0; offset < this->size(); ++offset)
        if (this->hashes[offset] != 0)
            GC.addValue(this->dtors[offset],this->values[offset],this->flags[offset]);
    *this = std::move(GC);
}
ResourceGC::ResourceGC(uint32_t count) {
    if (count < minimumSize())
        count = minimumSize();

    uint32_t offsets[5];
    offsets[0] = 0;
    offsets[1] = offsets[0] + alignTo64(sizeof(Type),count);
    offsets[2] = offsets[1] + alignTo64(sizeof(uint32_t),count);
    offsets[3] = offsets[2] + alignTo64(sizeof(dtor_fn*),count);
    offsets[4] = offsets[3] + alignTo64(sizeof(internal::GCFlag),count);

    uint8_t * const ptr = offsets[4] ? allocator<uint8_t>().allocate(offsets[4]) : nullptr;

    // must be: ptr + 0
    values.reset((Type*)        (ptr+offsets[0]));
    hashes = (uint32_t*)        (ptr+offsets[1]);
    dtors  = (dtor_fn**)        (ptr+offsets[2]);
    flags  = (internal::GCFlag*)(ptr+offsets[3]);
    _size = unoccupied = count;

    memset(this->hashes,0,alignTo64(sizeof(uint32_t),count));
    memset(flags,       0,alignTo64(sizeof(internal::GCFlag),count));
}
ResourceGC& ResourceGC::operator=(ResourceGC&& v) {
    if(this != &v) {
        this->~ResourceGC();
        memcpy(this,&v,sizeof(v));
        v.values.release();
        v.mitems = v._size = v.unoccupied = 0;uint32_t _size = 0;
        v.minValue=std::numeric_limits<Type>::max();
        v.maxValue=std::numeric_limits<Type>::min();
    }
    return *this;
}
ResourceGC::~ResourceGC() {
    for(uint32_t index=0;index<_size;++index)
        if(hashes[index] != 0)
            this->dtors[index](values[index]);
}
void ResourceGC::markValue(const Type value) {
    if (value < this->minValue || this->maxValue < value)
        return;
    uint32_t i, j, h;
    i = HashHelper::tgc_hash(value) % this->size(); j = 0;
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
void ResourceGC::sweep() {
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
