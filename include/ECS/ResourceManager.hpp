#if !defined(RESOURCEMANAGER_HPP)
#define RESOURCEMANAGER_HPP

#include "cutil/basics.hpp"
#include "cutil/set.hpp"
#include "Base/TypeID.hpp"
#include <vector>

namespace ECS {
    using ResourceID = uint32_t;
    struct Resource final {
        uint32_t index=0;
        ResourceID type=0;
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
        struct ResourceStore final {
            void          *Ctx = nullptr;
            LoadSignature *Load = nullptr;
            FreeSignature *Free = nullptr;
            uint32_t       typeSize = 0;
            uint32_t       blockCount = 0; 
            uint32_t       freeIndex = 0;
            ResourceID     type = 0;
            set<uint32_t>  hashmap;
            struct ResourceBlock {
                ResourceTask resources[Constants::ResourceBlockSize];
                uint32_t     refCounts[Constants::ResourceBlockSize];
                uint8_t      data[];
            };
            std::unique_ptr<ResourceBlock> blocks[Constants::ResourceBlockCount];

            ResourceTask& getResource(uint32_t index);
            uint32_t& getRefCount(uint32_t index);
            void* getValue(uint32_t index);
            void addBlock();
            ResourceTask& allocateResource(Hash32 hash);
            void freeResource(ResourceTask &task);
        };
        uint32_t                       typeCount = 0;
        std::unique_ptr<ResourceStore> sharedValues[Constants::MaximumResourcesCount];
        // ResourceManager();
    public:
        void loadResource(ResourceID id, string_view name, void *ctx, WaiterSignature *cb);
        void refCountIncrease(Resource r);
        void refCountDecrease(Resource r);
        void* getResource(Resource r);
        ResourceID registerResource(LoadSignature *load, FreeSignature *free, void* ctx, uint32_t typeSize);
    };
    template<typename> ResourceID __resourceid__() = delete;
    template<typename T>
    inline ResourceID getResourceID() { return __resourceid__<std::remove_const_t<std::remove_reference_t<T>>>(); }
}

#endif // RESOURCEMANAGER_HPP
