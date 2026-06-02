#if !defined(RESOURCEMANAGER_HPP)
#define RESOURCEMANAGER_HPP

#include "cutil/basics.hpp"
#include "Base/TypeID.hpp"
#include <vector>

namespace ECS {
    using ResourceType = uint32_t;
    struct Resource final {
        uint32_t index=0;
        ResourceType type=0;
    };
    typedef void(WaiterSignature)(void*,Resource);
    struct ResourceTask final {
        enum class ResourceState : uint32_t {
            Loaded = 0,
            Loading = 1,
            Failed = 2,
        };
        struct Waiter {
            void *ctx;
            WaiterSignature *cb;
        };
        ResourceState state;
        Resource res;
        Hash32 hash;
        std::vector<Waiter> waiters;
        void *value;
    };
    class ResourceManager final {
        typedef void (LoadSignature)(void*,ResourceTask*);
        typedef void (FreeSignature)(void*,ResourceTask&);
    private:
        struct ResourceStore;
        uint32_t                       typeCount = 0;
        ResourceStore *sharedValues[Constants::MaximumResourcesCount];
        // ResourceManager();
    public:
        ResourceManager(ResourceManager&&) = delete;
        ResourceManager& operator=(ResourceManager&&) = delete;
        ResourceManager()=default;
        ~ResourceManager();
        void loadResource(ResourceType id, string_view name, void *ctx, WaiterSignature *cb);
        void refCountIncrease(Resource r);
        void refCountDecrease(Resource r);
        void* getResource(Resource r);
        ResourceType registerResourceType(LoadSignature *load, FreeSignature *free, void* ctx, uint32_t typeSize);
    };
    template<typename> ResourceType __resourceid__() = delete;
    template<typename T>
    inline ResourceType getResourceID() { return __resourceid__<std::remove_const_t<std::remove_reference_t<T>>>(); }
}

#endif // RESOURCEMANAGER_HPP
