#if !defined(ISYSTEM_HPP)
#define ISYSTEM_HPP

#include "cutil/basics.hpp"
#include <vector>

namespace ECS
{
    struct DOE;
    struct ISystem;
    std::vector<ISystem*(*)(DOE&)>& _get_initialize_list();
    template<typename S>
    struct SystemRegister {
        SystemRegister() {
            std::vector<ISystem*(*)(DOE&)> &v = _get_initialize_list();
            v.emplace_back(&init);
        }
    private:
        static ISystem* init(DOE &e){
            S *ptr = std::allocator<S>().allocate(1);
            new (ptr) S(e);
            return ptr;
        }
    };
    struct ISystem {
        ISystem(DOE&){}
        virtual void OnUpdate(DOE&){};
        virtual void OnFixedUpdate(DOE&){};
        virtual void OnDestroy(DOE&){};
        virtual ~ISystem(){}
    };
}

#endif // ENTITY_HPP
