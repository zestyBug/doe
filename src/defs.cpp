#include "ECS/defs.hpp"
using namespace ECS;

StaticArray<ECS::comp_info,32> ECS::rtti;

comp_info ECS::_new_id(uint32_t size, rttiFP destructor, rttiFP constructor)
{
    TypeIndex ti;
    ti.value = rtti.size();
    if(size < 1)
        ti.value |= 0x2000;
    comp_info info{ti, size, destructor, constructor};
    rtti.push(info);
    return info;
}
