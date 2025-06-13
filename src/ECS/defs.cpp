#include "cutil/basics.hpp"
#include "ECS/defs.hpp"
#include "cutil/HashHelper.hpp"

namespace ECS {

static_array<comp_info,32> rtti;
static const comp_info &__FOR_ZERO_INDEXING_PURPOSE__= getTypeInfo<ECS::Entity>();

comp_info _new_id(uint32_t size, rttiFP destructor, rttiFP constructor)
{
    TypeID ti;
    ti.value = (uint16_t)rtti.size();
    if(unlikely(ti.value > (TypeID::MaxTypeCount-1)))
        throw std::bad_typeid();
    if(size < 1)
        ti.value |= (1 << 13);
    comp_info info{ti, size, destructor, constructor};
    rtti.push_back(info);
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

/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
[[nodiscard]] uint32_t operator""_hash32(const char *str, std::size_t count) noexcept {
    return HashHelper::FNV1A32((void*)str,count*sizeof(char));
}

/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
[[nodiscard]] uint32_t operator""_hash32(const wchar_t *str, std::size_t count) noexcept {
    return HashHelper::FNV1A32((void*)str,count*sizeof(wchar_t));
}

ssize_t allocator_counter = 0;
