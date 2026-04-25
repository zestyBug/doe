#if !defined(ISYSTEM_HPP)
#define ISYSTEM_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct DOE;
    struct ISystem {
        virtual void OnUpdate(DOE*){};
        virtual void OnUpdatePhysic(DOE*){};
        virtual ~ISystem(){}
    };
}

#endif // ENTITY_HPP
