#ifndef RESOURCE_G_C_HPP
#define RESOURCE_G_C_HPP 1

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "defs.hpp"
#include "ECS/EntityComponentManager.hpp"

namespace ECS 
{

class EntityComponentManager;

enum class CGFlag : uint8_t {
    NONE = 0x0,
    MARK = 0x1,
    ROOT = 0x2,
    LEAF = 0x4
};

inline constexpr CGFlag operator&(CGFlag x, CGFlag y) {
    return static_cast<CGFlag>(static_cast<uint8_t>(x) & static_cast<uint8_t>(y));
}
inline constexpr CGFlag operator|(CGFlag x, CGFlag y) {
    return static_cast<CGFlag>(static_cast<uint8_t>(x) | static_cast<uint8_t>(y));
}
inline constexpr CGFlag operator~(CGFlag x) { 
    return static_cast<CGFlag>(~static_cast<uint8_t>(x));
}
inline CGFlag & operator&=(CGFlag & x, CGFlag y){ x = x & y;return x; }
inline CGFlag & operator|=(CGFlag & x, CGFlag y) { x = x | y;return x; }

/// @brief stores set of values. garbage collects values with mark & sweep algorithm, searching inside ECMs.
/// @tparam invalide_value default and invalid value to init array of 'Type'
template <typename Type,Type invalide_value>
class ResourceGC
{
public:
    typedef void (dtor_fn)(Type);
protected:
    Type* values = nullptr;
    dtor_fn** dtors = nullptr;
    CGFlag* flags = nullptr;
    uint32_t _size = 0;
    uint32_t unoccupied = 0;
    const const_span<ECS::TypeID> components;
    Type minValue=UINTPTR_MAX, maxValue=0;
public:
    ResourceGC(const_span<ECS::TypeID> _components,uint32_t count = 0) : components{_components} {
        if (count < minimumSize())
            count = minimumSize();
        // isnt power of 2?
    #ifdef DEBUG
        if(components.size() < 1)
            throw std::invalid_argument("ResourceGC(): component(s) count must be GE 1");
        if(count < 2)
            throw std::invalid_argument("ResourceGC(): array size must be atleast 2");
    #endif
        if(0 != (count & (count - 1)))
            throw std::invalid_argument("ResourceGC(): array size must be power of 2");

        uint32_t offsets[4];
        offsets[0] = 0;
        offsets[1] = offsets[0] + alignTo64(sizeof(Type),count);
        offsets[2] = offsets[1] + alignTo64(sizeof(dtor_fn*),count);
        offsets[3] = offsets[2] + alignTo64(sizeof(CGFlag),count);

        uint8_t * const ptr = allocator<uint8_t>().allocate(offsets[3]);

        // must be: ptr + 0
        values = (Type*)(ptr+offsets[0]);
        dtors  = (dtor_fn**)(ptr+offsets[1]);
        flags  = (CGFlag*)(ptr+offsets[2]);
        _size = unoccupied = count;

        for(auto& v:span(values,count))
            v = invalide_value;
        memset(flags,0,sizeof(CGFlag)*count);
    }
    ResourceGC(const ResourceGC&) = delete;
    ResourceGC(ResourceGC&& v){
        *this = std::move(v);
    }
    ResourceGC& operator=(const ResourceGC&) = delete;
    ResourceGC& operator=(ResourceGC&& v) {
        if(this != &v){
            this->values = v.values;
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
            if(const Type v = values[index];v != invalide_value)
                this->dtors[index](v);
        allocator<uint8_t>().deallocate(values);
    }
    inline uint32_t size() const { return this->_size; }
    inline uint32_t unoccupiedNodes() const { return unoccupied; }
    inline uint32_t occupiedNodes() const { return size() - unoccupiedNodes();}
    inline bool isEmpty() const { return occupiedNodes() == 0; }
    /// @brief ! suppose size is power of 2
    inline uint32_t hashMask() const { return size() - 1; }
    static inline uint32_t minimumSize() { return std::max<uint32_t>(64u / sizeof(Type),4u); }
    inline void possiblyGrow() {
        if (unoccupiedNodes() < size() / 3){
            const uint32_t new_size = size() * 2;
            // otherwise indexOf may return invalid value!
            if(new_size < INT32_MAX)
                resize(new_size);
        }
    }
    inline void possiblyShrink() {
        if (occupiedNodes() < size() / 3)
            resize(size() / 2);
    }
    inline bool contains(Type value) const { return indexOf(value) != -1; }

    void appendFrom(const ResourceGC& src) {
        for (uint32_t offset = 0; offset < src.size(); ++offset) {
            const Type value = src.values[offset];
            if (value != invalide_value)
                insert(value,src.dtors[offset],src.flags[offset]);
        }
    }
    void resize(uint32_t size) {
        if (size < minimumSize())
            size = minimumSize();
        if (size == this->size())
            return;
        ResourceGC temp(this->components,size);
        temp.appendFrom(*this);
        *this = std::move(temp);
    }
    void insert(Type value,dtor_fn* dtor, CGFlag flag = CGFlag::NONE) {
        if(value == invalide_value)
            throw std::invalid_argument("insert(): invalide argument");

        this->minValue = std::min(value,this->minValue);
        this->maxValue = std::max(value,this->maxValue);
        
        uint32_t hmask = hashMask();
        uint32_t offset = (int)(value & hmask);
        uint32_t attempts = 0;
        while (true)
        {
            const Type v = values[offset];
            if(v == value)
            {
                this->dtors[offset] = dtor;
                this->flags[offset] = flag;
                return;
            }
            else if (v == invalide_value)
            {
                this->values[offset] = value;
                this->dtors[offset] = dtor;
                this->flags[offset] = flag;
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
        if(value == invalide_value)
            throw std::invalid_argument("insert(): invalide argument");
        uint32_t offset = (value & hashMask());
        uint32_t attempts = 0;
        while (true)
        {
            const Type v = values[offset];
            if (v == value)
                return offset;
            if (v == invalide_value)
                return -1;
            offset = (offset + 1) & hashMask();
            ++attempts;
            if (attempts == size())
                return -1;
        }
    }
    // the whole popuse is to free space when object is unused but still in memory
    void reset(){
        for(auto& v:span(this->values,this->count))
            v = invalide_value;
        memset(this->flags,0,this->count * sizeof(uint16_t));
        this->unoccupied = this->count;
        minValue=UINTPTR_MAX;
        maxValue=0;
    }
protected:
    void markValue(const Type value)
    {
        if (value < this->minValue || this->maxValue < value)
            return; 
        int i = indexOf(value);
        if(i >= 0){
            if (static_cast<uint8_t>(flags[i] & CGFlag::MARK))
                return;
            flags[i] |= CGFlag::MARK;
        }
    }
    void searchMemory(const uint8_t *region,const uint32_t regionSizeInByte) {
        for (uint32_t k = 0; k < regionSizeInByte/sizeof(Type); k++)
            markValue(((const Type*)region)[k]);
    }
    void iterateChunks(Archetype const * const archetype, 
            uint32_t numBuffers, 
            const uint16_t* sizeBuffer, 
            const uint32_t* offsetBuffer) {
        const_span<Chunk> archetypeChunks   = archetype->chunksData;
        const uint32_t lastChunkEntityCount = archetype->lastChunkEntityCount;
        const uint32_t chunkCapacity        = archetype->chunkCapacity;
        // may underflow!
        const uint32_t lastChunkIndex       = archetypeChunks.size() - 1;

    #ifdef DEBUG
        if(unlikely(  chunkCapacity == 0 || 
            (archetypeChunks.size() > 0 && lastChunkEntityCount == 0 )
        )) throw std::runtime_error("iterateChunks(): bad archetype!");
    #endif

        for(uint32_t chunckIndex = 0; chunckIndex < archetypeChunks.size(); chunckIndex++)
        {
            const uint8_t *chunkMemory = (const uint8_t *)archetypeChunks[chunckIndex].memory;
            const uint32_t entityCount = chunckIndex == lastChunkIndex ? lastChunkEntityCount : chunkCapacity ;
        #ifdef DEBUG
            if(unlikely(chunkMemory == nullptr))
                throw std::runtime_error("iterateChunks(): found an uninitialized chunk in archetype!");
        #endif
            for (size_t index = 0; index < numBuffers; index++)
                searchMemory(chunkMemory + offsetBuffer[index], entityCount * sizeBuffer[index]);
        }
    }
    void searchArchetype(Archetype const * const archetype)
    {
        uint32_t number_of_found = 0;
        uint16_t size_buffer[this->components.size()];
        uint32_t offset_buffer[this->components.size()];
        {
            const_span<uint16_t> archetypeSizes;
            const_span<uint32_t> archetypeOffsets;
            const_span<TypeID> archetypeTypes;
            uint32_t archetypeTypeIndex = 0;
            uint32_t componentIndex = 0;

            archetypeOffsets = archetype->offsets;
            archetypeTypes = archetype->types;
            archetypeSizes = archetype->sizeOfs;
            // fill offset_buffer
            for(; archetypeTypeIndex < archetypeTypes.size() && componentIndex < this->components.size();)
            {
                if(this->components[componentIndex].value == archetypeTypes[archetypeTypeIndex].value)
                {
                    size_buffer[number_of_found] = archetypeSizes[archetypeTypeIndex];
                    offset_buffer[number_of_found] = archetypeOffsets[archetypeTypeIndex];
                    ++number_of_found;
                    ++componentIndex;
                    ++archetypeTypeIndex;
                }
                else if(this->components[componentIndex].value < archetypeTypes[archetypeTypeIndex].value)
                    ++componentIndex;
                else
                    ++archetypeTypeIndex;
            }
        }
        if(number_of_found)
            iterateChunks(archetype,number_of_found,size_buffer,offset_buffer);
    }
public:
    void mark(const EntityComponentManager* ecm) {
        const Archetype *arch;
        uint32_t archetypeIndex;
        for (archetypeIndex = 0; archetypeIndex < ecm->archetypes.size(); archetypeIndex++)
            if(arch = ecm->archetypes[archetypeIndex].get())
                searchArchetype(arch);
    }
    void sweep() {
        uint32_t i, j, k, nj, nh;
        if (this->isEmpty()) { return; }
        for (i = 0; i < this->size(); i++) {
            if (this->values[i] == invalide_value) { continue; }
            if (static_cast<uint8_t>(this->flags[i] & CGFlag::MARK)) { continue; }
            if (static_cast<uint8_t>(this->flags[i] & CGFlag::ROOT)) { continue; }
            this->dtors[i](this->values[i]);
        }
        for (i = 0; i < this->size(); i++) {
            if (this->values[i] == invalide_value) { continue; }
            if (static_cast<uint8_t>(this->flags[i] & CGFlag::MARK))
            {
                this->flags[i] &= ~CGFlag::MARK;
                continue;
            }
            if (static_cast<uint8_t>(this->flags[i] & CGFlag::ROOT)) { continue; }
            this->values[i] = invalide_value;
            this->unoccupied++;
        }
    }
};

}

#endif