#include "cutil/basics.hpp"
#include "ECS/defs.hpp"
#include "cutil/HashHelper.hpp"

namespace ECS {

size_t internal::rtti_count = 1;
comp_info internal::rtti[COMPOMEN_COUNT] = {
    TypeID{0},
    sizeof(ECS::Entity),
    nullptr,
    nullptr
};

comp_info internal::_new_id(uint32_t size, rttiFP destructor, rttiFP constructor)
{
    TypeID ti;
    ti.value = (uint16_t)rtti_count;
    if(unlikely(ti.value > (TypeID::MaxTypeCount-1)))
        throw std::bad_typeid();
    if(size < 1)
        // MAGIC NUMBER
        ti.value |= (1 << 13);
    const comp_info info{ti, size, destructor, constructor};
    rtti[rtti_count++] = info;
    return info;
}
bool didChange(version_t version, version_t lastVersion)
{
    // if is allocated recently
    if (version == 0)
        return true;
    // overflow detection for longer run
    return (version - lastVersion) < (1u << 31);
}
void updateVersion(version_t& lastVersion){
    lastVersion++;
    // 0 is reserved
    if(lastVersion==0)
        lastVersion=1;
}

}

ssize_t allocator_counter = 0;
