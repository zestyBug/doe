#if !defined(ENTITY_HPP)
#define ENTITY_HPP

#include <vector>
#include <algorithm>    // std::sort
#include "cutil/basics.hpp"

namespace ECS
{
    struct Chunk;
    // this engine uses uint32 and int32 for all cases,
    // unless it is specified.

    struct Entity final {
        constexpr Entity():_value{Null}{}
        constexpr Entity(const int32_t v,uint32_t version=0):_value{v},_version{version}{}
        constexpr Entity(const Entity&) = default;
        constexpr Entity(Entity&&) = default;
        inline Entity& operator = (const Entity&) = default;
        inline bool operator == (const Entity& v) const {return this->index() == v.index() && this->_version == v._version;}
        inline bool operator != (const Entity& v) const {return !(*this == v);}
        ~Entity() = default;
        // index in entity_value array
        inline int32_t index() const {return this->_value;}
        inline uint32_t version() const {return this->_version;}
        // entity id (index in entity_Value array)
        static constexpr int32_t Null = -1;
        // simply checks entity index validity
        inline bool isValid() const {return this->index() >= 0;}
    protected:
        int32_t _value;
        uint32_t _version=0;
    };

    struct EntityInChunk final {
        Chunk* chunk = nullptr;
        uint32_t indexInChunk = 0;
        inline bool isNull() {return chunk == nullptr;}
        EntityInChunk() = default;
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

    struct EntityBatchInChunk {
        Chunk* chunk;
        uint32_t startIndex;
        uint32_t count;
    };
}

#endif // ENTITY_HPP
