#ifndef RESOURCE_G_C_HPP
#define RESOURCE_G_C_HPP 1

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "cutil/HashHelper.hpp"
#include "Base/TypeID.hpp"
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
}

/// @brief stores set of values. garbage collects values with mark & sweep algorithm, searching inside ECMs.
class ResourceGC
{
public:
    using Type = intptr_t;
    // MAGIC NUMBER
    static const uint32_t INVALID_INDEX = 0xFFFFFFFF;
    typedef void (dtor_fn)(Type);
protected:
    align_ptr<Type[]> values{};
    uint32_t* hashes = nullptr;
    dtor_fn** dtors = nullptr;
    internal::GCFlag* flags = nullptr;
    uint32_t _size = 0;
    uint32_t mitems = 0;
    uint32_t unoccupied = 0;
    Type minValue=std::numeric_limits<Type>::max();
    Type maxValue=std::numeric_limits<Type>::min();
    float loadFactor = 0.9f;
    float sweepFactor = 0.5f;

    /// @brief probes index of the hash in the array for sorting hashes
    /// @return original offset from the desired hash location
    uint32_t probe(uint32_t i, uint32_t h) const ;
    uint32_t getIndex(Type value) const ;
    void addValue(dtor_fn* dtor, Type value, internal::GCFlag flag = internal::GCFlag::NONE);
    void remIndex(uint32_t j);
    void remValue(Type value);
    uint32_t idealSize(uint32_t current_size);
    /// @brief move values to new array with new size
    /// @param size new size
    /// @note it doesnt update max/min value
    /// @warning resizing to smaller size that occupiedNodes leads to undefined behavior
    void rehash(uint32_t size);
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
    ResourceGC(uint32_t count = 0);
    ResourceGC(const ResourceGC&) = delete;
    ResourceGC(ResourceGC&& v) : values{nullptr},_size{0},unoccupied{0} {
        *this = std::move(v);
    }
    ResourceGC& operator=(const ResourceGC&) = delete;
    ResourceGC& operator=(ResourceGC&& v);
    ~ResourceGC();

    inline uint32_t size() const { return this->_size; }
    inline uint32_t unoccupiedNodes() const { return unoccupied; }
    inline uint32_t occupiedNodes() const { return size() - unoccupiedNodes();}
    inline bool isEmpty() const { return occupiedNodes() == 0; }
    static inline uint32_t minimumSize() { return 0; }
    inline bool contains(Type value) const { return getIndex(value) != INVALID_INDEX; }
    bool recommendGC() const { return this->occupiedNodes() > this->mitems; }


protected:
    void markValue(const Type value);
public:
    void searchMemory(const uint8_t *region,const uint32_t regionSizeInByte) {
        for (uint32_t k = 0; k < regionSizeInByte/sizeof(Type); k++)
            markValue(((const Type*)region)[k]);
    }
    void sweep();
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
