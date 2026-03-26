#if !defined(VERSION_HPP)
#define VERSION_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    // this engine uses uint32 and int32 for all cases,
    // unless it is specified.

    struct Version {
        uint32_t value;

        constexpr Version():value{0}{}
        constexpr Version(const uint32_t v):value{v}{}
        constexpr Version(const Version&) = default;
        constexpr Version(Version&&) = default;
        constexpr Version& operator = (const Version&) = default;
        constexpr bool operator == (const Version& v) const {return this->value == v.value;}
        constexpr bool operator != (const Version& v) const {return this->value != v.value;}

        inline operator uint32_t& () {
            return value;
        }
        inline operator uint32_t () const {
            return value;
        }

        /// @brief returns true if component contains newer version than system (been modofied)
        /// @param last vesion of system that been run
        /// @return true diffrence is greater than zero
        bool didChange(Version last) const {
            // if is allocated recently
            if (value == 0)
                return true;
            // overflow detection for longer run
            return (value - last.value) < (1u << 31);
        }
        void updateVersion() {
            value++;
            // 0 is reserved
            if(value==0)
                value=1;
        }
    };
}

#endif // ENTITY_HPP
