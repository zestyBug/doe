#if !defined(QUERY_HPP)
#define QUERY_HPP

#include "TypeID.hpp"

namespace ECS {
    struct EntityQueryData;

    struct EntityQueryBuilder {
        static const uint32_t capacity = 32;
        EntityQueryBuilder() = default;
        EntityQueryBuilder(const  EntityQueryBuilder&) = default;
        void withAll(TypeID type){
            if(count >= capacity)
                throw std::out_of_range("ArchetypeQuery: out of space");
            uint32_t index = count++;
            _all[index] = type;
            _flags[index] = 0;
        }
        void withAllRW(TypeID type){
            if(count >= capacity)
                throw std::out_of_range("ArchetypeQuery: out of space");
            uint32_t index = count++;
            _all[index] = type;
            _flags[index] = WriteFlag;
        }
        void withAny(TypeID type){
            if(count >= capacity)
                throw std::out_of_range("ArchetypeQuery: out of space");
            uint32_t index = count++;
            _all[index] = type;
            _flags[index] = AnyFlag;
        }
        void withAnyRW(TypeID type){
            if(count >= capacity)
                throw std::out_of_range("ArchetypeQuery: out of space");
            uint32_t index = count++;
            _all[index] = type;
            _flags[index] = AnyFlag | WriteFlag;
        }
        void withNone(TypeID type){
            if(count >= capacity)
                throw std::out_of_range("ArchetypeQuery: out of space");
            uint32_t index = count++;
            _all[index] = type;
            _flags[index] = NoneFlag;
        }
    private:
        friend class EntityQueryManager;
        friend class ComponentDependencyManager;
        static const uint16_t WriteFlag = 1;
        static const uint16_t AnyFlag = 1 << 1;
        static const uint16_t NoneFlag = 1 << 2;
        uint32_t count = 0;
        TypeID _all[capacity];
        uint8_t _flags[capacity];
    };
    struct EntityQueryImpl {
        friend struct EntityQueryManager;
        EntityQueryImpl() = default;
        EntityQueryImpl(const EntityQueryImpl&) = default;
        EntityQueryImpl& operator = (const EntityQueryImpl&) = default;
        EntityQueryImpl& operator = (EntityQueryImpl&&) = default;
        EntityQueryImpl(EntityQueryImpl &&) = default;
        ~EntityQueryImpl() = default;
        EntityQueryData* getData();
    private:
        EntityQueryImpl(EntityQueryData *ptr):queryData{ptr}{};
        EntityQueryData *queryData = nullptr;
    };
}

#endif // QUERY_HPP
