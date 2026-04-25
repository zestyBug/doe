#if !defined(ARCHETYPEQUERY_HPP)
#define ARCHETYPEQUERY_HPP

#include "TypeID.hpp"

namespace ECS {
    class ComponentDependencyManager;
    // CreateEntityQuery, AddArchetypeIfMatching
    struct ArchetypeQuery {
        void withAll(TypeID type){
            if(count[1] >= 32)
                throw std::out_of_range("ArchetypeQuery: out of space");
            _all[count[1]++] = (uint16_t)type.index();
        }
        void withAllRW(TypeID type){
            if(count[1] >= 32)
                throw std::out_of_range("ArchetypeQuery: out of space");
            _all[count[1]++] = (uint16_t)type.index() | WriteFlag;
        }
        void withAny(TypeID type){
            if(count[2] >= 32)
                throw std::out_of_range("ArchetypeQuery: out of space");
            _any[count[2]++] = (uint16_t)type.index();
        }
        void withAnyRW(TypeID type){
            if(count[2] >= 32)
                throw std::out_of_range("ArchetypeQuery: out of space");
            _any[count[2]++] = (uint16_t)type.index() | WriteFlag;
        }
        void withNone(TypeID type){
            if(count[3] >= 32)
                throw std::out_of_range("ArchetypeQuery: out of space");
            _none[count[3]++] = (uint16_t)type.index();
        }
        void sort(){
            std::sort(this->_all,this->_all+this->count[1],comp);
            std::sort(this->_any,this->_any+this->count[2],comp);
            std::sort(this->_none,this->_none+this->count[3],comp);
        }
    private:
        static bool comp(uint16_t a, uint16_t b){
            return (a & IndexMask) < (b & IndexMask);
        }
        friend class EntityQueryManager;
        friend class ComponentDependencyManager;
        static const uint32_t WriteFlag = 1 << 15;
        static const uint32_t IndexMask = 0x7fff;
        /// @brief 1: ALL, 2: ANY, 3: NONE
        uint16_t count[4] = {3,0,0,0};
        uint16_t _all[32];
        uint16_t _any[32];
        uint16_t _none[32];
    };
}

#endif // ARCHETYPEQUERY_HPP
