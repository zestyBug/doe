#include "ECS/defs.hpp"
using namespace ECS;

StaticArray<ECS::comp_info,32> ECS::rtti;

comp_info ECS::_new_id(uint32_t size, rttiFP destructor, rttiFP constructor)
{
    TypeID ti;
    ti.value = (uint16_t)rtti.size();
    if(unlikely(ti.value > (TypeID::maxTypeCount-1)))
        throw std::bad_typeid();
    if(size < 1)
        ti.value |= (1 << 13);
    comp_info info{ti, size, destructor, constructor};
    rtti.push_back(info);
    return info;
}
