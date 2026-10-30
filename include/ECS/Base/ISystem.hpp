#if !defined(ISYSTEM_HPP)
#define ISYSTEM_HPP

#include "cutil/basics.hpp"
#include <vector>

namespace ECS
{
    struct Engine;
    struct ISystem;
    std::vector<ISystem*(*)(Engine&)>& _get_initialize_list();
    template<typename S>
    struct SystemRegister {
        SystemRegister() {
            std::vector<ISystem*(*)(Engine&)> &v = _get_initialize_list();
            v.emplace_back(&init);
        }
    private:
        static ISystem* init(Engine &e){
            S *ptr = std::allocator<S>().allocate(1);
            new (ptr) S(e);
            return ptr;
        }
    };
    struct ISystem {
        ISystem(Engine&){}
        virtual void BeforeUpdate(Engine&){};
        virtual void OnUpdate(Engine&){};
        virtual void AfterUpdate(Engine&){};
        virtual void OnFixedUpdate(Engine&){};
        virtual void OnDestroy(Engine&){};
        virtual ~ISystem(){}
    };
}

#endif // ENTITY_HPP
