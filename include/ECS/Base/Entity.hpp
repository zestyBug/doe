#if !defined(ENTITY_HPP)
#define ENTITY_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct Chunk;
    // this engine uses uint32 and int32 for all cases,
    // unless it is specified.

    struct Entity final {
        constexpr Entity() = default;
        constexpr Entity(const uint32_t v,uint32_t version=0):_value{v},_version{version}{}
        constexpr Entity(const Entity&) = default;
        constexpr Entity(Entity&&) = default;
        inline Entity& operator = (const Entity&) = default;
        inline bool operator == (const Entity& v) const {return this->index() == v.index() && this->_version == v._version;}
        inline bool operator != (const Entity& v) const {return !(*this == v);}
        ~Entity() = default;
        // index in entity_value array
        inline uint32_t index()   const {return this->_value;}
        inline uint32_t version() const {return this->_version;}
        inline bool     isValid() const {return this->_version & 1;}
    protected:
        uint32_t _value=0;
        uint32_t _version=0;
    };

    /// @brief Entity --> EntityStore --EntityInChunk--> Chunk --> Archetype
    struct EntityInChunk final {
        Chunk* chunk = nullptr;
        uint32_t indexInChunk = 0;
        inline bool isNull() {return chunk == nullptr;}
        bool operator == (const EntityInChunk& other) const {
            return this->chunk == other.chunk && this->indexInChunk == other.indexInChunk;
        }
    };

    struct EntityName {
        int8_t value[16];
        static const EntityName* Null(){
            static const EntityName v = {0};
            return &v;
        }
    };

    struct EntityBatchInChunk final {
        Chunk* chunk;
        uint32_t startIndex;
        uint32_t count;
    };
}

#endif // ENTITY_HPP
