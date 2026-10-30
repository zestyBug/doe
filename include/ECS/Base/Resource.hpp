#if !defined(RESOURCE_HPP)
#define RESOURCE_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct ResourceEntry;
    /// @brief Basically a pointer to ResourceEntry plus version flag
    struct Resource {
        Resource() = default;
    private:
        ResourceEntry* getAddress() const {
            return (ResourceEntry*)(((intptr_t)this->ptr >> 4) << 4);
        }
        uint16_t getVersion(){
            return (intptr_t)this->ptr & 0b1111;
        }
        friend class ResourceManager;
        void *ptr = nullptr;
    };
}

#endif // RESOURCE_HPP
